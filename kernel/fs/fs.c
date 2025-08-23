#include "fs.h"

static int fs_initialized = 0;
static unsigned int fs_current_dir_lba = 0;
static FS_SuperOnDisk fs_super;

static int read_sector(unsigned int lba, void *buf)
{
    if (!fs_drv || !fs_drv->exists)
        return FS_ERR_NO_DRV;

    int r = fs_disk_read(lba, 1, buf);
    if (r != 0)
        return FS_ERR_IO;

    return FS_OK;
}

static int write_sector(unsigned int lba, const void *buf)
{
    if (!fs_drv || !fs_drv->exists)
        return FS_ERR_NO_DRV;

    int r = fs_disk_write(lba, 1, (void *)buf);
    if (r != 0)
        return FS_ERR_IO;

    return FS_OK;
}

static unsigned int dir_index_to_lba(unsigned int idx)
{
    return DIR_TABLE_START_LBA + idx;
}

static unsigned int file_index_to_lba(unsigned int idx)
{
    return FILE_TABLE_START_LBA + idx;
}

static int lba_to_dir_index(unsigned int lba)
{
    if (lba < DIR_TABLE_START_LBA)
        return -1;

    unsigned int idx = lba - DIR_TABLE_START_LBA;
    if (idx >= MAX_DIRS)
        return -1;

    return (int)idx;
}

static int lba_to_file_index(unsigned int lba)
{
    if (lba < FILE_TABLE_START_LBA)
        return -1;

    unsigned int idx = lba - FILE_TABLE_START_LBA;
    if (idx >= MAX_FILES)
        return -1;

    return (int)idx;
}

static int load_super(void)
{
    unsigned char buf[512];

    int r = read_sector(SUPERBLOCK_LBA, buf);
    if (r != FS_OK)
        return r;

    mem_copy(&fs_super, buf, sizeof(FS_SuperOnDisk));

    if (fs_super.magic != FS_MAGIC)
        return FS_ERR_IO;

    if (fs_super.dir_table_start < DIR_TABLE_START_LBA)
        return FS_ERR_IO;

    if (fs_super.file_table_start < FILE_TABLE_START_LBA)
        return FS_ERR_IO;

    if (fs_super.data_start < FILE_TABLE_START_LBA)
        return FS_ERR_IO;

    return FS_OK;
}

static int store_super(void)
{
    unsigned char buf[512];

    mem_set(buf, 0, 512);
    mem_copy(buf, &fs_super, sizeof(FS_SuperOnDisk));

    int r = write_sector(SUPERBLOCK_LBA, buf);
    if (r != FS_OK)
        return r;

    unsigned char verify[512];
    if (read_sector(SUPERBLOCK_LBA, verify) != FS_OK)
        return FS_ERR_IO;

    unsigned int vm;
    mem_copy(&vm, verify, sizeof(unsigned int));
    if (vm != FS_MAGIC)
        return FS_ERR_IO;

    return FS_OK;
}

static int read_dir_lba(unsigned int lba, FS_Dir *d)
{
    unsigned char buf[512];

    int r = read_sector(lba, buf);
    if (r != FS_OK)
        return r;

    mem_copy(d, buf, sizeof(FS_Dir));

    return FS_OK;
}

static int write_dir_lba(unsigned int lba, const FS_Dir *d)
{
    unsigned char buf[512];

    mem_set(buf, 0, 512);
    mem_copy(buf, d, sizeof(FS_Dir));

    return write_sector(lba, buf);
}

static int read_file_lba(unsigned int lba, FS_File *f)
{
    unsigned char buf[512];

    int r = read_sector(lba, buf);
    if (r != FS_OK)
        return r;

    mem_copy(f, buf, sizeof(FS_File));
    return FS_OK;
}

static int write_file_lba(unsigned int lba, const FS_File *f)
{
    unsigned char buf[512];

    mem_set(buf, 0, 512);
    mem_copy(buf, f, sizeof(FS_File));

    return write_sector(lba, buf);
}

static int find_free_dir_index(void)
{
    for (unsigned int i = 0; i < MAX_DIRS; i++)
        if (fs_super.dir_bitmap[i] == 0)
            return (int)i;

    return -1;
}

static int find_free_file_index(void)
{
    for (unsigned int i = 0; i < MAX_FILES; i++)
        if (fs_super.file_bitmap[i] == 0)
            return (int)i;

    return -1;
}

static void set_dir_bitmap(int idx, unsigned char val)
{
    if (idx >= 0 && idx < (int)MAX_DIRS)
        fs_super.dir_bitmap[idx] = val ? 1 : 0;
}

static void set_file_bitmap(int idx, unsigned char val)
{
    if (idx >= 0 && idx < (int)MAX_FILES)
        fs_super.file_bitmap[idx] = val ? 1 : 0;
}

void fs_init(void)
{
    static ATA_Driver drv;

    clear();
    write("fs_init: start.\n");
    if (ata_init(&drv, 0x1F0, 0x3F6, 0) != 0)
    {
        write_colored("fs_init: ATA init failed.\n", 0x04);

        fs_initialized = 0;
        fs_drv = 0;
        return;
    }

    fs_drv = &drv;
    write_colored("fs_init: ATA init ok.\n", 0x02);

    if (!fs_drv->exists)
    {
        write_colored("fs_init: no ata device.\n", 0x04);

        fs_initialized = 0;
        return;
    }

    write("fs_init: loading super.\n");
    if (load_super() == FS_OK)
    {
        fs_current_dir_lba = fs_super.root_dir_lba;
        fs_initialized = 1;

        write_colored("fs_init: using existing FS on disk.\n\n", 0x02);
        return;
    }

    write_colored("fs_init: no valid super - formatting new FS.\n", 0x02);

    mem_set(&fs_super, 0, sizeof(FS_SuperOnDisk));

    fs_super.magic = FS_MAGIC;
    fs_super.max_dirs = MAX_DIRS;
    fs_super.max_files = MAX_FILES;
    fs_super.dir_table_start = DIR_TABLE_START_LBA;
    fs_super.file_table_start = FILE_TABLE_START_LBA;
    fs_super.data_start = DATA_START_LBA;

    mem_set(fs_super.dir_bitmap, 0, MAX_DIRS);
    mem_set(fs_super.file_bitmap, 0, MAX_FILES);

    int root_idx = 0;
    unsigned int root_lba = dir_index_to_lba((unsigned int)root_idx);

    FS_Dir root;
    mem_set(&root, 0, sizeof(FS_Dir));
    str_copy_fixed(root.name, "/", MAX_NAME);
    root.parent_lba = root_lba;
    root.dir_count = 0;
    root.file_count = 0;

    write("fs_init: writing root dir.\n");
    if (write_dir_lba(root_lba, &root) != FS_OK)
    {
        write_colored("fs_init: failed to write root dir.\n", 0x04);

        fs_initialized = 0;
        return;
    }

    set_dir_bitmap(root_idx, 1);
    fs_super.root_dir_lba = root_lba;

    write("fs_init: storing super.\n");
    if (store_super() != FS_OK)
    {
        write_colored("fs_init: failed to write/verify super\n", 0x04);

        fs_initialized = 0;
        return;
    }

    fs_current_dir_lba = root_lba;
    fs_initialized = 1;

    write_colored("fs_init: created new FS.\n\n", 0x02);
}

const char *fs_get_current_dir_name(void)
{
    static FS_Dir cur;
    if (!fs_initialized)
        return "(fs not init)";

    if (read_dir_lba(fs_current_dir_lba, &cur) != FS_OK)
        return "(io error)";

    return cur.name;
}

int fs_make_dir(const char *name)
{
    if (!fs_initialized)
        return FS_ERR_NOT_INIT;

    if (str_count(name) >= MAX_NAME)
        return FS_ERR_NAME_LONG;

    if (str_count(name) == 0)
        return FS_ERR_NO_NAME;

    FS_Dir cur;
    if (read_dir_lba(fs_current_dir_lba, &cur) != FS_OK)
        return FS_ERR_IO;

    if ((int)cur.dir_count >= MAX_DIRS_PER_DIR)
        return FS_ERR_NO_SPACE;

    for (unsigned int i = 0; i < cur.dir_count; i++)
    {
        unsigned int child_lba = cur.dirs_lba[i];
        FS_Dir tmp;

        if (read_dir_lba(child_lba, &tmp) != FS_OK)
            return FS_ERR_IO;

        if (str_equal(tmp.name, name))
            return FS_ERR_EXISTS;
    }

    for (unsigned int i = 0; i < cur.file_count; i++)
    {
        unsigned int child_lba = cur.files_lba[i];
        FS_File tmp;

        if (read_file_lba(child_lba, &tmp) != FS_OK)
            return FS_ERR_IO;

        if (str_equal(tmp.name, name))
            return FS_ERR_EXISTS;
    }

    int new_idx = find_free_dir_index();
    if (new_idx < 0)
        return FS_ERR_NO_SPACE;

    unsigned int new_lba = dir_index_to_lba((unsigned int)new_idx);

    FS_Dir nd;
    mem_set(&nd, 0, sizeof(FS_Dir));
    str_copy_fixed(nd.name, name, MAX_NAME);
    nd.parent_lba = fs_current_dir_lba;
    nd.dir_count = 0;
    nd.file_count = 0;

    if (write_dir_lba(new_lba, &nd) != FS_OK)
        return FS_ERR_IO;

    set_dir_bitmap(new_idx, 1);
    if (store_super() != FS_OK)
        return FS_ERR_IO;

    cur.dirs_lba[cur.dir_count] = new_lba;
    cur.dir_count++;

    if (write_dir_lba(fs_current_dir_lba, &cur) != FS_OK)
        return FS_ERR_IO;

    return FS_OK;
}

int fs_make_file(const char *name)
{
    if (!fs_initialized)
        return FS_ERR_NOT_INIT;

    if (str_count(name) >= MAX_NAME)
        return FS_ERR_NAME_LONG;

    if (str_count(name) == 0)
        return FS_ERR_NO_NAME;

    FS_Dir cur;
    if (read_dir_lba(fs_current_dir_lba, &cur) != FS_OK)
        return FS_ERR_IO;

    if ((int)cur.file_count >= MAX_FILES_PER_DIR)
        return FS_ERR_NO_SPACE;

    for (unsigned int i = 0; i < cur.dir_count; i++)
    {
        unsigned int child_lba = cur.dirs_lba[i];
        FS_Dir tmp;

        if (read_dir_lba(child_lba, &tmp) != FS_OK)
            return FS_ERR_IO;

        if (str_equal(tmp.name, name))
            return FS_ERR_EXISTS;
    }

    for (unsigned int i = 0; i < cur.file_count; i++)
    {
        unsigned int child_lba = cur.files_lba[i];
        FS_File tmp;

        if (read_file_lba(child_lba, &tmp) != FS_OK)
            return FS_ERR_IO;

        if (str_equal(tmp.name, name))
            return FS_ERR_EXISTS;
    }

    int new_idx = find_free_file_index();
    if (new_idx < 0)
        return FS_ERR_NO_SPACE;

    unsigned int new_lba = file_index_to_lba((unsigned int)new_idx);

    FS_File nf;
    mem_set(&nf, 0, sizeof(FS_File));
    str_copy_fixed(nf.name, name, MAX_NAME);
    nf.size = 0;
    nf.data_lba = 0;

    if (write_file_lba(new_lba, &nf) != FS_OK)
        return FS_ERR_IO;

    set_file_bitmap(new_idx, 1);
    if (store_super() != FS_OK)
        return FS_ERR_IO;

    cur.files_lba[cur.file_count] = new_lba;
    cur.file_count++;

    if (write_dir_lba(fs_current_dir_lba, &cur) != FS_OK)
        return FS_ERR_IO;

    return FS_OK;
}

void fs_list_dir(void)
{
    if (!fs_initialized)
    {
        write_colored("Error: File system not initialized.\n", 0x04);
        return;
    }

    FS_Dir cur;
    if (read_dir_lba(fs_current_dir_lba, &cur) != FS_OK)
    {
        write_colored("Error: I/O error.\n", 0x04);
        return;
    }

    int printed = 0;
    for (unsigned int i = 0; i < cur.dir_count; i++)
    {
        unsigned int child_lba = cur.dirs_lba[i];
        FS_Dir tmp;

        if (read_dir_lba(child_lba, &tmp) != FS_OK)
            continue;

        char out[32];
        unsigned int ni = 0;
        for (unsigned int k = 0; k < MAX_NAME && tmp.name[k]; k++)
            out[ni++] = tmp.name[k];

        out[ni++] = ' ';
        out[ni] = '\0';

        write_colored(out, 0x09);
        printed = 1;
    }

    for (unsigned int i = 0; i < cur.file_count; i++)
    {
        unsigned int child_lba = cur.files_lba[i];
        FS_File tmp;

        if (read_file_lba(child_lba, &tmp) != FS_OK)
            continue;

        char out[32];
        unsigned int ni = 0;

        for (unsigned int k = 0; k < MAX_NAME && tmp.name[k]; k++)
            out[ni++] = tmp.name[k];

        out[ni++] = ' ';
        out[ni] = '\0';

        write(out);
        printed = 1;
    }

    if (!printed)
        write("(empty directory)\n");
    else
        write("\n");
}

int fs_change_dir(const char *name)
{
    if (!fs_initialized)
        return FS_ERR_NOT_INIT;

    if (str_count(name) == 0)
        return FS_ERR_NO_NAME;

    FS_Dir cur;
    if (read_dir_lba(fs_current_dir_lba, &cur) != FS_OK)
        return FS_ERR_IO;

    if (str_equal(name, ".."))
    {
        if (fs_current_dir_lba == fs_super.root_dir_lba)
            return FS_OK;

        fs_current_dir_lba = cur.parent_lba;
        return FS_OK;
    }

    for (unsigned int i = 0; i < cur.dir_count; i++)
    {
        unsigned int child_lba = cur.dirs_lba[i];
        FS_Dir tmp;

        if (read_dir_lba(child_lba, &tmp) != FS_OK)
            return FS_ERR_IO;

        if (str_equal(tmp.name, name))
        {
            fs_current_dir_lba = child_lba;
            return FS_OK;
        }
    }

    return FS_ERR_NOT_EXISTS;
}

void fs_where(void)
{
    if (!fs_initialized)
    {
        write_colored("Error: File system not initialized.\n", 0x04);
        return;
    }

    unsigned int path_lbas[MAX_DIRS];
    unsigned int depth = 0;
    unsigned int lba = fs_current_dir_lba;

    while (1)
    {
        path_lbas[depth++] = lba;
        FS_Dir cur;

        if (read_dir_lba(lba, &cur) != FS_OK)
        {
            write_colored("Error: I/O error.\n", 0x04);
            return;
        }

        if (lba == cur.parent_lba)
            break;

        lba = cur.parent_lba;
    }

    write("/");

    for (int i = depth - 2; i >= 0; i--)
    {
        FS_Dir cur;
        if (read_dir_lba(path_lbas[i], &cur) != FS_OK)
        {
            write_colored("Error: I/O error.\n", 0x04);
            return;
        }

        write(cur.name);

        if (i > 0)
            write("/");
    }

    write("\n");
}

int fs_delete_dir(const char *name)
{
    if (!fs_initialized)
        return FS_ERR_NOT_INIT;

    if (str_count(name) == 0)
        return FS_ERR_NO_NAME;

    FS_Dir cur;
    if (read_dir_lba(fs_current_dir_lba, &cur) != FS_OK)
        return FS_ERR_IO;

    for (unsigned int i = 0; i < cur.dir_count; i++)
    {
        unsigned int child_lba = cur.dirs_lba[i];
        FS_Dir tmp;

        if (read_dir_lba(child_lba, &tmp) != FS_OK)
            return FS_ERR_IO;

        if (str_equal(tmp.name, name))
        {
            if (tmp.dir_count > 0 || tmp.file_count > 0)
                return FS_ERR_EXISTS;

            int idx = lba_to_dir_index(child_lba);
            if (idx >= 0)
                set_dir_bitmap(idx, 0);

            if (store_super() != FS_OK)
                return FS_ERR_IO;

            for (unsigned int j = i; j + 1 < cur.dir_count; j++)
                cur.dirs_lba[j] = cur.dirs_lba[j + 1];

            cur.dir_count--;

            if (write_dir_lba(fs_current_dir_lba, &cur) != FS_OK)
                return FS_ERR_IO;

            return FS_OK;
        }
    }

    return FS_ERR_NOT_EXISTS;
}

int fs_delete_file(const char *name)
{
    if (!fs_initialized)
        return FS_ERR_NOT_INIT;

    if (str_count(name) == 0)
        return FS_ERR_NO_NAME;

    FS_Dir cur;
    if (read_dir_lba(fs_current_dir_lba, &cur) != FS_OK)
        return FS_ERR_IO;

    for (unsigned int i = 0; i < cur.file_count; i++)
    {
        unsigned int child_lba = cur.files_lba[i];
        FS_File tmp;

        if (read_file_lba(child_lba, &tmp) != FS_OK)
            return FS_ERR_IO;

        if (str_equal(tmp.name, name))
        {
            int idx = lba_to_file_index(child_lba);
            if (idx >= 0)
                set_file_bitmap(idx, 0);

            if (store_super() != FS_OK)
                return FS_ERR_IO;

            for (unsigned int j = i; j + 1 < cur.file_count; j++)
                cur.files_lba[j] = cur.files_lba[j + 1];

            cur.file_count--;

            if (write_dir_lba(fs_current_dir_lba, &cur) != FS_OK)
                return FS_ERR_IO;

            return FS_OK;
        }
    }

    return FS_ERR_NOT_EXISTS;
}

int fs_write_file(const char *name, const char *text)
{
    if (!fs_initialized)
        return FS_ERR_NOT_INIT;

    if (str_count(name) == 0)
        return FS_ERR_NO_NAME;

    if (str_count(text) == 0)
        return FS_ERR_NO_TEXT;

    FS_Dir cur;
    if (read_dir_lba(fs_current_dir_lba, &cur) != FS_OK)
        return FS_ERR_IO;

    for (unsigned int i = 0; i < cur.file_count; i++)
    {
        unsigned int file_lba = cur.files_lba[i];
        FS_File f;

        if (read_file_lba(file_lba, &f) != FS_OK)
            return FS_ERR_IO;

        if (str_equal(f.name, name))
        {
            unsigned int len = str_count(text);
            if (len > 512)
                len = 512;

            unsigned char buf[512];
            for (unsigned int j = 0; j < len; j++)
                buf[j] = text[j];

            for (unsigned int j = len; j < 512; j++)
                buf[j] = 0;

            f.size = len;
            if (f.data_lba == 0)
                f.data_lba = DATA_START_LBA + file_lba;

            if (write_sector(f.data_lba, buf) != FS_OK)
                return FS_ERR_IO;

            if (write_file_lba(file_lba, &f) != FS_OK)
                return FS_ERR_IO;

            return FS_OK;
        }
    }

    return FS_ERR_NOT_EXISTS;
}

int fs_read_file(const char *name)
{
    if (!fs_initialized)
        return FS_ERR_NOT_INIT;

    if (str_count(name) == 0)
        return FS_ERR_NO_NAME;

    FS_Dir cur;
    if (read_dir_lba(fs_current_dir_lba, &cur) != FS_OK)
        return FS_ERR_IO;

    for (unsigned int i = 0; i < cur.file_count; i++)
    {
        unsigned int file_lba = cur.files_lba[i];
        FS_File f;

        if (read_file_lba(file_lba, &f) != FS_OK)
            return FS_ERR_IO;

        if (str_equal(f.name, name))
        {
            if (f.data_lba == 0 || f.size == 0)
            {
                write("(empty file)\n");
                return FS_OK;
            }

            unsigned char buf[512];
            if (read_sector(f.data_lba, buf) != FS_OK)
                return FS_ERR_IO;

            for (unsigned int j = 0; j < f.size; j++)
                write_char(buf[j]);

            write("\n");
            return FS_OK;
        }
    }

    return FS_ERR_NOT_EXISTS;
}

void fs_info(const char *name)
{
    if (!fs_initialized)
    {
        write_colored("Error: File system not initialized.\n", 0x04);
        return;
    }

    if (str_count(name) == 0)
    {
        write_colored("Error: No such file or directory.\n", 0x04);
        return;
    }

    FS_Dir cur;
    if (read_dir_lba(fs_current_dir_lba, &cur) != FS_OK)
    {
        write_colored("Error: I/O error occured while reading info.\n", 0x04);
        return;
    }

    for (unsigned int i = 0; i < cur.file_count; i++)
    {
        unsigned int file_lba = cur.files_lba[i];
        FS_File f;

        if (read_file_lba(file_lba, &f) != FS_OK)
            continue;

        if (str_equal(f.name, name))
        {
            write("File: ");
            write(f.name);
            write("\n");
            write("Size: ");
            write(uint_to_str(f.size));
            write(" bytes\n");
            write("In directory: ");
            write(cur.name);
            write("\n");

            return;
        }
    }

    for (unsigned int i = 0; i < cur.dir_count; i++)
    {
        unsigned int dir_lba = cur.dirs_lba[i];
        FS_Dir d;

        if (read_dir_lba(dir_lba, &d) != FS_OK)
            continue;

        if (str_equal(d.name, name))
        {
            write("Directory: ");
            write(d.name);
            write("\n");

            FS_Dir parent;
            if (read_dir_lba(d.parent_lba, &parent) != FS_OK)
            {
                write_colored("Error: Parent read error.\n", 0x04);
                return;
            }

            write("Parent directory: ");
            write(parent.name);
            write("\n");

            write("Files: ");
            write(uint_to_str(d.file_count));
            write("\n");
            write("Subdirectories: ");
            write(uint_to_str(d.dir_count));
            write("\n");

            return;
        }
    }

    write_colored("Error: No such file or directory.\n", 0x04);
}