#include "fs.h"
#include "../drivers/display/display.h"
#include "../drivers/pit/pit.h"
#include "../drivers/disk/ata.h"
#include "../helpers/string/string.h"
#include "../helpers/memory/memory.h"
#include "../helpers/ports/ports.h"
#include "../paging/heap/heap.h"

#define FS_MAGIC 0x46535953u // 'FSYS'

#define SUPERBLOCK_LBA 2048u
#define DIR_TABLE_START_LBA (SUPERBLOCK_LBA + 1u)
#define DIR_TABLE_SECTORS (MAX_DIRS)
#define FILE_TABLE_START_LBA (DIR_TABLE_START_LBA + DIR_TABLE_SECTORS)
#define FILE_TABLE_SECTORS (MAX_FILES)
#define DATA_START_LBA (FILE_TABLE_START_LBA + FILE_TABLE_SECTORS)

#define START_DELAY 2000

static int fs_initialized = 0;

static unsigned int fs_current_dir_lba = 0;
static FS_SuperOnDisk fs_super;

static int read_sector(unsigned int lba, void *buf)
{
    ATA_Driver *fs_drv = ata_get_fs_drv();

    if (!fs_drv || !fs_drv->exists)
        return FS_ERR_NO_DRV;

    int r = fs_disk_read(lba, 1, buf);
    if (r != 0)
        return FS_ERR_IO;

    return FS_OK;
}

static int write_sector(unsigned int lba, const void *buf)
{
    ATA_Driver *fs_drv = ata_get_fs_drv();

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

static int contains_slash(const char *s)
{
    if (!s)
        return 0;
    unsigned int n = str_count(s);
    for (unsigned int i = 0; i < n; i++)
        if (s[i] == '/')
            return 1;
    return 0;
}

static int get_next_component(const char *path, unsigned int *pos, char *out, unsigned int out_sz)
{
    unsigned int n = str_count(path);
    unsigned int i = *pos;

    while (i < n && path[i] == '/')
        i++;

    if (i >= n)
    {
        out[0] = '\0';
        *pos = i;
        return 0;
    }

    unsigned int start = i;
    unsigned int len = 0;
    while (i < n && path[i] != '/')
    {
        if (len + 1 >= out_sz)
            return -1;
        out[len++] = path[i++];
    }
    out[len] = '\0';
    *pos = i;

    return 1;
}

static int has_more_components(const char *path, unsigned int pos)
{
    unsigned int n = str_count(path);
    unsigned int i = pos;
    while (i < n)
    {
        if (path[i] == '/')
        {
            i++;
            continue;
        }
        return 1;
    }
    return 0;
}

static int find_child_dir_lba(const FS_Dir *parent_dir, const char *name, unsigned int *out_lba)
{
    for (unsigned int i = 0; i < parent_dir->dir_count; i++)
    {
        unsigned int child_lba = parent_dir->dirs_lba[i];
        FS_Dir tmp;
        if (read_dir_lba(child_lba, &tmp) != FS_OK)
            return FS_ERR_IO;
        if (str_equal(tmp.name, name))
        {
            *out_lba = child_lba;
            return FS_OK;
        }
    }
    return FS_ERR_NOT_EXISTS;
}

static int resolve_path(const char *path, unsigned int *out_dir_lba, char *out_name, int stop_before_last)
{
    if (!fs_initialized)
        return FS_ERR_NOT_INIT;

    unsigned int pos = 0;
    unsigned int n = str_count(path);

    if (n == 0)
    {
        if (stop_before_last)
        {
            out_name[0] = '\0';
            *out_dir_lba = fs_current_dir_lba;
        }
        else
        {
            *out_dir_lba = fs_current_dir_lba;
            if (out_name)
                out_name[0] = '\0';
        }
        return FS_OK;
    }

    unsigned int cur_lba;
    if (path[0] == '/')
    {
        cur_lba = fs_super.root_dir_lba;
        pos = 1;
    }
    else
    {
        cur_lba = fs_current_dir_lba;
        pos = 0;
    }

    char token[MAX_NAME];
    int rc;
    int produced;

    while (1)
    {
        produced = get_next_component(path, &pos, token, sizeof(token));
        if (produced == 0)
        {
            if (stop_before_last)
            {
                out_name[0] = '\0';
                *out_dir_lba = cur_lba;
                return FS_OK;
            }
            else
            {
                *out_dir_lba = cur_lba;
                if (out_name)
                    out_name[0] = '\0';
                return FS_OK;
            }
        }
        if (produced == -1)
            return FS_ERR_NAME_LONG;

        int more = has_more_components(path, pos);
        int is_last = (more == 0);

        if (stop_before_last && is_last)
        {
            str_copy_fixed(out_name, token, MAX_NAME);
            *out_dir_lba = cur_lba;
            return FS_OK;
        }

        if (str_equal(token, "."))
            continue;

        else if (str_equal(token, ".."))
        {
            FS_Dir curdir;
            if (read_dir_lba(cur_lba, &curdir) != FS_OK)
                return FS_ERR_IO;
            cur_lba = curdir.parent_lba;
            continue;
        }
        else
        {
            FS_Dir curdir;
            if (read_dir_lba(cur_lba, &curdir) != FS_OK)
                return FS_ERR_IO;
            unsigned int child_lba = 0;
            rc = find_child_dir_lba(&curdir, token, &child_lba);
            if (rc != FS_OK)
                return FS_ERR_NOT_EXISTS;
            cur_lba = child_lba;
            continue;
        }
    }

    return FS_ERR_IO;
}

int fs_is_initialized(void)
{
    return fs_initialized;
}

void fs_init(void)
{
    static ATA_Driver drv_inst;
    ATA_Driver *drv = &drv_inst;

    write("Initializing file system...\n");
    if (ata_init(drv, 0x1F0, 0x3F6, 0) != 0)
    {
        fs_initialized = 0;
        ata_set_fs_drv(((void *)0));
        return;
    }

    ata_set_fs_drv(drv);

    write("Loading super...\n");
    if (load_super() == FS_OK)
    {
        fs_current_dir_lba = fs_super.root_dir_lba;
        fs_initialized = 1;

        write("\033[32mUsing existing file system on disk.\033[0m\n\n");
        pit_sleep(START_DELAY);
        clear();

        return;
    }

    write("\033[32mNo valid super found. Formatting a new file system.\033[0m\n");
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

    write("Writing root directory...\n");
    if (write_dir_lba(root_lba, &root) != FS_OK)
    {
        write("\033[31mError: Failed to write root directory. Halting...\033[0m\n\n");

        fs_initialized = 0;

        __asm__ volatile(
            "cli\n\t"
            "hlt\n\t");
        while (1)
            ;
    }

    set_dir_bitmap(root_idx, 1);
    fs_super.root_dir_lba = root_lba;

    write("Storing super...\n");
    if (store_super() != FS_OK)
    {
        write("\033[31mError: Failed to write or verify super. Halting...\033[0m\n\n");

        fs_initialized = 0;

        __asm__ volatile(
            "cli\n\t"
            "hlt\n\t");
        while (1)
            ;
    }

    fs_current_dir_lba = root_lba;
    fs_initialized = 1;

    write("\033[32mCreated a new file system.\033[0m\n\n");

    pit_sleep(START_DELAY);
    clear();
}

int fs_make_dir(const char *name)
{
    if (!fs_initialized)
        return FS_ERR_NOT_INIT;

    if (!contains_slash(name) && str_count(name) >= MAX_NAME)
        return FS_ERR_NAME_LONG;

    char final_name[MAX_NAME];
    unsigned int parent_lba = 0;
    int rc = resolve_path(name, &parent_lba, final_name, 1);
    if (rc != FS_OK)
        return rc;

    if (str_count(final_name) == 0)
        return FS_ERR_NO_NAME;

    FS_Dir cur;
    if (read_dir_lba(parent_lba, &cur) != FS_OK)
        return FS_ERR_IO;

    if ((int)cur.dir_count >= MAX_DIRS_PER_DIR)
        return FS_ERR_NO_SPACE;

    for (unsigned int i = 0; i < cur.dir_count; i++)
    {
        unsigned int child_lba = cur.dirs_lba[i];
        FS_Dir tmp;

        if (read_dir_lba(child_lba, &tmp) != FS_OK)
            return FS_ERR_IO;

        if (str_equal(tmp.name, final_name))
            return FS_ERR_EXISTS;
    }

    for (unsigned int i = 0; i < cur.file_count; i++)
    {
        unsigned int child_lba = cur.files_lba[i];
        FS_File tmp;

        if (read_file_lba(child_lba, &tmp) != FS_OK)
            return FS_ERR_IO;

        if (str_equal(tmp.name, final_name))
            return FS_ERR_EXISTS;
    }

    int new_idx = find_free_dir_index();
    if (new_idx < 0)
        return FS_ERR_NO_SPACE;

    unsigned int new_lba = dir_index_to_lba((unsigned int)new_idx);

    FS_Dir nd;
    mem_set(&nd, 0, sizeof(FS_Dir));
    str_copy_fixed(nd.name, final_name, MAX_NAME);
    nd.parent_lba = parent_lba;
    nd.dir_count = 0;
    nd.file_count = 0;

    if (write_dir_lba(new_lba, &nd) != FS_OK)
        return FS_ERR_IO;

    set_dir_bitmap(new_idx, 1);
    if (store_super() != FS_OK)
        return FS_ERR_IO;

    cur.dirs_lba[cur.dir_count] = new_lba;
    cur.dir_count++;

    if (write_dir_lba(parent_lba, &cur) != FS_OK)
        return FS_ERR_IO;

    return FS_OK;
}

int fs_make_file(const char *name)
{
    if (!fs_initialized)
        return FS_ERR_NOT_INIT;

    if (!contains_slash(name) && str_count(name) >= MAX_NAME)
        return FS_ERR_NAME_LONG;

    char final_name[MAX_NAME];
    unsigned int parent_lba = 0;
    int rc = resolve_path(name, &parent_lba, final_name, 1);
    if (rc != FS_OK)
        return rc;

    if (str_count(final_name) == 0)
        return FS_ERR_NO_NAME;

    FS_Dir cur;
    if (read_dir_lba(parent_lba, &cur) != FS_OK)
        return FS_ERR_IO;

    if ((int)cur.file_count >= MAX_FILES_PER_DIR)
        return FS_ERR_NO_SPACE;

    for (unsigned int i = 0; i < cur.dir_count; i++)
    {
        unsigned int child_lba = cur.dirs_lba[i];
        FS_Dir tmp;

        if (read_dir_lba(child_lba, &tmp) != FS_OK)
            return FS_ERR_IO;

        if (str_equal(tmp.name, final_name))
            return FS_ERR_EXISTS;
    }

    for (unsigned int i = 0; i < cur.file_count; i++)
    {
        unsigned int child_lba = cur.files_lba[i];
        FS_File tmp;

        if (read_file_lba(child_lba, &tmp) != FS_OK)
            return FS_ERR_IO;

        if (str_equal(tmp.name, final_name))
            return FS_ERR_EXISTS;
    }

    int new_idx = find_free_file_index();
    if (new_idx < 0)
        return FS_ERR_NO_SPACE;

    unsigned int new_lba = file_index_to_lba((unsigned int)new_idx);

    FS_File nf;
    mem_set(&nf, 0, sizeof(FS_File));
    str_copy_fixed(nf.name, final_name, MAX_NAME);
    nf.size = 0;
    nf.data_lba = 0;

    if (write_file_lba(new_lba, &nf) != FS_OK)
        return FS_ERR_IO;

    set_file_bitmap(new_idx, 1);
    if (store_super() != FS_OK)
        return FS_ERR_IO;

    cur.files_lba[cur.file_count] = new_lba;
    cur.file_count++;

    if (write_dir_lba(parent_lba, &cur) != FS_OK)
        return FS_ERR_IO;

    return FS_OK;
}

char *fs_list_dir(void)
{
    if (!fs_initialized)
    {
        char *err = kmalloc(32);
        if (!err)
            return ((void *)0);
        str_copy_fixed(err, "Error: File system not initialized.\n", 32);
        return err;
    }

    FS_Dir cur;
    if (read_dir_lba(fs_current_dir_lba, &cur) != FS_OK)
    {
        char *err = kmalloc(24);
        if (!err)
            return ((void *)0);
        str_copy_fixed(err, "Error: I/O error.\n", 24);
        return err;
    }

    unsigned int est_size = 0;
    for (unsigned int i = 0; i < cur.dir_count; i++)
    {
        FS_Dir tmp;
        if (read_dir_lba(cur.dirs_lba[i], &tmp) == FS_OK)
            est_size += str_count(tmp.name) + 2;
    }
    for (unsigned int i = 0; i < cur.file_count; i++)
    {
        FS_File tmp;
        if (read_file_lba(cur.files_lba[i], &tmp) == FS_OK)
            est_size += str_count(tmp.name) + 1;
    }
    if (est_size == 0)
        est_size = 20;
    est_size += 2;

    char *output = kmalloc(est_size);
    if (!output)
        return ((void *)0);

    int pos = 0;
    int printed = 0;

    for (unsigned int i = 0; i < cur.dir_count; i++)
    {
        FS_Dir tmp;
        if (read_dir_lba(cur.dirs_lba[i], &tmp) != FS_OK)
            continue;

        for (unsigned int k = 0; tmp.name[k]; k++)
            output[pos++] = tmp.name[k];

        output[pos++] = '/';
        output[pos++] = ' ';
        printed = 1;
    }

    for (unsigned int i = 0; i < cur.file_count; i++)
    {
        FS_File tmp;
        if (read_file_lba(cur.files_lba[i], &tmp) != FS_OK)
            continue;

        for (unsigned int k = 0; tmp.name[k]; k++)
            output[pos++] = tmp.name[k];

        output[pos++] = ' ';
        printed = 1;
    }

    if (!printed)
        str_copy_fixed(output, "(empty directory)\n", est_size);
    else
        output[pos++] = '\n';

    output[pos] = '\0';
    return output;
}

int fs_change_dir(const char *name)
{
    if (!fs_initialized)
        return FS_ERR_NOT_INIT;

    if (str_count(name) == 0)
        return FS_ERR_NO_NAME;

    unsigned int dir_lba = 0;
    char dummy[MAX_NAME];
    int rc = resolve_path(name, &dir_lba, dummy, 0);
    if (rc != FS_OK)
        return rc;

    fs_current_dir_lba = dir_lba;
    return FS_OK;
}

char *fs_where(void)
{
    if (!fs_initialized)
    {
        char *err = kmalloc(32);
        if (!err)
            return ((void *)0);
        str_copy_fixed(err, "Error: File system not initialized.\n", 32);
        return err;
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
            char *err = kmalloc(24);
            if (!err)
                return ((void *)0);
            str_copy_fixed(err, "Error: I/O error.\n", 24);
            return err;
        }

        if (lba == cur.parent_lba)
            break;

        lba = cur.parent_lba;
    }

    unsigned int est_size = 1;
    for (int i = depth - 2; i >= 0; i--)
    {
        FS_Dir tmp;
        if (read_dir_lba(path_lbas[i], &tmp) != FS_OK)
            continue;
        est_size += str_count(tmp.name) + 1;
    }
    est_size += 1;

    char *path = kmalloc(est_size);
    if (!path)
        return ((void *)0);

    int pos = 0;
    path[pos++] = '/';
    path[pos] = '\0';

    for (int i = depth - 2; i >= 0; i--)
    {
        FS_Dir tmp;
        if (read_dir_lba(path_lbas[i], &tmp) != FS_OK)
        {
            kfree(path);
            char *err = kmalloc(24);
            if (!err)
                return ((void *)0);
            str_copy_fixed(err, "Error: I/O error.\n", 24);
            return err;
        }

        for (int j = 0; tmp.name[j]; j++)
            path[pos++] = tmp.name[j];

        if (i > 0)
            path[pos++] = '/';

        path[pos] = '\0';
    }

    return path;
}

int fs_delete_dir(const char *name)
{
    if (!fs_initialized)
        return FS_ERR_NOT_INIT;

    if (str_count(name) == 0)
        return FS_ERR_NO_NAME;

    char final_name[MAX_NAME];
    unsigned int parent_lba = 0;
    int rc = resolve_path(name, &parent_lba, final_name, 1);
    if (rc != FS_OK)
        return rc;

    if (str_count(final_name) == 0)
        return FS_ERR_NO_NAME;

    FS_Dir cur;
    if (read_dir_lba(parent_lba, &cur) != FS_OK)
        return FS_ERR_IO;

    for (unsigned int i = 0; i < cur.dir_count; i++)
    {
        unsigned int child_lba = cur.dirs_lba[i];
        FS_Dir tmp;

        if (read_dir_lba(child_lba, &tmp) != FS_OK)
            return FS_ERR_IO;

        if (str_equal(tmp.name, final_name))
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

            if (write_dir_lba(parent_lba, &cur) != FS_OK)
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

    char final_name[MAX_NAME];
    unsigned int parent_lba = 0;
    int rc = resolve_path(name, &parent_lba, final_name, 1);
    if (rc != FS_OK)
        return rc;

    if (str_count(final_name) == 0)
        return FS_ERR_NO_NAME;

    FS_Dir cur;
    if (read_dir_lba(parent_lba, &cur) != FS_OK)
        return FS_ERR_IO;

    for (unsigned int i = 0; i < cur.file_count; i++)
    {
        unsigned int file_lba = cur.files_lba[i];
        FS_File f;
        if (read_file_lba(file_lba, &f) != FS_OK)
            return FS_ERR_IO;

        if (str_equal(f.name, final_name))
        {
            if (f.data_lba != 0 && f.size > 0)
            {
                unsigned char zero[512];
                mem_set(zero, 0, sizeof(zero));
                unsigned int sectors = (f.size + 511) / 512;
                for (unsigned int s = 0; s < sectors; s++)
                    if (write_sector(f.data_lba + s, zero) != FS_OK)
                        return FS_ERR_IO;
            }

            mem_set(&f, 0, sizeof(FS_File));
            if (write_file_lba(file_lba, &f) != FS_OK)
                return FS_ERR_IO;

            int idx = lba_to_file_index(file_lba);
            if (idx >= 0)
                set_file_bitmap(idx, 0);

            if (store_super() != FS_OK)
                return FS_ERR_IO;

            for (unsigned int j = i; j + 1 < cur.file_count; j++)
                cur.files_lba[j] = cur.files_lba[j + 1];
            cur.file_count--;

            if (write_dir_lba(parent_lba, &cur) != FS_OK)
                return FS_ERR_IO;

            return FS_OK;
        }
    }

    return FS_ERR_NOT_EXISTS;
}

int fs_write_file(const char *name, const unsigned char *data, unsigned int size)
{
    if (!fs_initialized)
        return FS_ERR_NOT_INIT;

    if (!name || str_count(name) == 0)
        return FS_ERR_NO_NAME;

    if (size == 0)
        return FS_OK;

    char final_name[MAX_NAME];
    unsigned int parent_lba = 0;
    int rc = resolve_path(name, &parent_lba, final_name, 1);
    if (rc != FS_OK)
        return rc;

    if (str_count(final_name) == 0)
        return FS_ERR_NO_NAME;

    FS_Dir cur;
    if (read_dir_lba(parent_lba, &cur) != FS_OK)
        return FS_ERR_IO;

    for (unsigned int i = 0; i < cur.file_count; i++)
    {
        unsigned int file_lba = cur.files_lba[i];
        FS_File f;
        if (read_file_lba(file_lba, &f) != FS_OK)
            return FS_ERR_IO;

        if (str_equal(f.name, final_name))
        {
            int file_idx = lba_to_file_index(file_lba);
            if (file_idx < 0)
                return FS_ERR_IO;

            if (f.data_lba == 0)
                f.data_lba = DATA_START_LBA + (unsigned int)file_idx;

            unsigned int written = 0;
            unsigned int sector_lba = f.data_lba + (f.size / 512u);
            unsigned int sector_off = f.size % 512u;
            unsigned char buf[512];

            if (sector_off != 0)
            {
                if (read_sector(sector_lba, buf) != FS_OK)
                    return FS_ERR_IO;
                unsigned int can = 512 - sector_off;
                unsigned int to_copy = (size - written < can) ? (size - written) : can;
                for (unsigned int j = 0; j < to_copy; j++)
                    buf[sector_off + j] = data[written + j];
                if (write_sector(sector_lba, buf) != FS_OK)
                    return FS_ERR_IO;
                written += to_copy;
                sector_lba++;
            }

            while (written + 512 <= size)
            {
                for (unsigned int j = 0; j < 512; j++)
                    buf[j] = data[written + j];
                if (write_sector(sector_lba, buf) != FS_OK)
                    return FS_ERR_IO;
                sector_lba++;
                written += 512;
            }

            if (written < size)
            {
                unsigned int rem = size - written;
                for (unsigned int j = 0; j < rem; j++)
                    buf[j] = data[written + j];
                for (unsigned int j = rem; j < 512; j++)
                    buf[j] = 0;
                if (write_sector(sector_lba, buf) != FS_OK)
                    return FS_ERR_IO;
                written += rem;
            }

            f.size = f.size + size;
            if (write_file_lba(file_lba, &f) != FS_OK)
                return FS_ERR_IO;

            if (store_super() != FS_OK)
                return FS_ERR_IO;

            return FS_OK;
        }
    }

    return FS_ERR_NOT_EXISTS;
}

int fs_read_file(const char *name, unsigned char *out_buf, unsigned int buf_size, unsigned int *out_size)
{
    if (!fs_initialized)
        return FS_ERR_NOT_INIT;

    if (str_count(name) == 0)
        return FS_ERR_NO_NAME;

    char final_name[MAX_NAME];
    unsigned int parent_lba = 0;
    int rc = resolve_path(name, &parent_lba, final_name, 1);
    if (rc != FS_OK)
        return rc;

    if (str_count(final_name) == 0)
        return FS_ERR_NO_NAME;

    FS_Dir cur;
    if (read_dir_lba(parent_lba, &cur) != FS_OK)
        return FS_ERR_IO;

    for (unsigned int i = 0; i < cur.file_count; i++)
    {
        unsigned int file_lba = cur.files_lba[i];
        FS_File f;

        if (read_file_lba(file_lba, &f) != FS_OK)
            return FS_ERR_IO;

        if (str_equal(f.name, final_name))
        {
            if (f.data_lba == 0 || f.size == 0)
            {
                if (out_size)
                    *out_size = 0;
                return FS_OK;
            }

            unsigned char buf[512];
            unsigned int remaining = f.size;
            unsigned int written = 0;
            unsigned int sector_lba = f.data_lba;

            while (remaining > 0)
            {
                if (read_sector(sector_lba, buf) != FS_OK)
                    return FS_ERR_IO;

                unsigned int to_copy = remaining < 512 ? remaining : 512;

                if (out_buf && buf_size > 0)
                {
                    unsigned int can_copy = (buf_size - written) < to_copy ? (buf_size - written) : to_copy;
                    for (unsigned int j = 0; j < can_copy; j++)
                        out_buf[written + j] = buf[j];
                    written += can_copy;
                }
                else
                    written += to_copy;

                remaining -= to_copy;
                sector_lba++;
            }

            if (out_size)
                *out_size = written;

            return FS_OK;
        }
    }

    return FS_ERR_NOT_EXISTS;
}
