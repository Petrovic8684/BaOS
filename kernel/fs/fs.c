#include "fs.h"

Directory root;
Directory *current_dir;
Directory all_dirs[32];
File all_files[64];
int next_dir;
int next_file;

void fs_init(void)
{
    current_dir = &root;
    for (int i = 0; i < MAX_NAME; i++)
        root.name[i] = 0;
    root.name[0] = '/';
    root.parent = (void *)0;
    root.dir_count = 0;
    root.file_count = 0;

    next_dir = 0;
    next_file = 0;
}

const char *fs_get_current_dir_name(void)
{
    return current_dir->name;
}

void fs_make_dir(const char *name)
{
    if (name[0] == 0)
    {
        write("Directory name cannot be empty!\n");
        return;
    }

    if (current_dir->dir_count >= MAX_DIRS)
    {
        write("Directory limit reached!\n");
        return;
    }

    if (current_dir->subdirs)
    {
        for (int i = 0; i < current_dir->dir_count; i++)
        {
            if (str_equal(current_dir->subdirs[i]->name, name))
            {
                write("Directory already exists!\n");
                return;
            }
        }
    }

    if (str_equal(current_dir->name, name))
    {
        write("Cannot create directory with the same name as the current directory!\n");
        return;
    }

    Directory *d = &all_dirs[next_dir++];
    int i;

    for (i = 0; i < MAX_NAME; i++)
        d->name[i] = 0;

    for (i = 0; i < MAX_NAME - 1 && name[i]; i++)
        d->name[i] = name[i];

    d->parent = current_dir;
    d->dir_count = 0;
    d->file_count = 0;
    current_dir->subdirs[current_dir->dir_count++] = d;

    write("Directory created!\n");
}

void fs_delete_dir(const char *name)
{
    if (name[0] == 0)
    {
        write("Directory name cannot be empty!\n");
        return;
    }

    for (int i = 0; i < current_dir->dir_count; i++)
    {
        if (str_equal(current_dir->subdirs[i]->name, name))
        {
            Directory *d = current_dir->subdirs[i];
            if (d->dir_count > 0 || d->file_count > 0)
            {
                write("Directory not empty!\n");
                return;
            }

            for (int j = i; j < current_dir->dir_count - 1; j++)
                current_dir->subdirs[j] = current_dir->subdirs[j + 1];

            current_dir->dir_count--;
            write("Directory deleted!\n");
            return;
        }
    }
    write("Directory not found!\n");
}

void fs_delete_file(const char *name)
{
    if (name[0] == 0)
    {
        write("File name cannot be empty!\n");
        return;
    }

    for (int i = 0; i < current_dir->file_count; i++)
    {
        if (str_equal(current_dir->files[i]->name, name))
        {
            for (int j = i; j < current_dir->file_count - 1; j++)
                current_dir->files[j] = current_dir->files[j + 1];

            current_dir->file_count--;
            write("File deleted!\n");
            return;
        }
    }
    write("File not found!\n");
}

void fs_make_file(const char *name)
{
    if (name[0] == 0)
    {
        write("File name cannot be empty!\n");
        return;
    }

    if (current_dir->file_count >= MAX_FILES)
    {
        write("File limit reached!\n");
        return;
    }

    if (current_dir->files)
    {
        for (int i = 0; i < current_dir->file_count; i++)
        {
            if (str_equal(current_dir->files[i]->name, name))
            {
                write("File already exists!\n");
                return;
            }
        }
    }

    File *f = &all_files[next_file++];

    for (int i = 0; i < MAX_NAME; i++)
        f->name[i] = 0;

    int i = 0;
    while (i < MAX_NAME - 1 && name[i])
    {
        f->name[i] = name[i];
        i++;
    }

    f->size = 0;
    f->content[0] = 0;
    current_dir->files[current_dir->file_count++] = f;

    write("File created!\n");
}

void fs_list_dir(void)
{
    if (current_dir->dir_count == 0 && current_dir->file_count == 0)
    {
        write("Empty directory!\n");
        return;
    }

    for (int i = 0; i < current_dir->dir_count; i++)
    {
        write_colored(current_dir->subdirs[i]->name, 0x01);
        write(" ");
    }

    for (int i = 0; i < current_dir->file_count; i++)
    {
        write(current_dir->files[i]->name);
        write(" ");
    }

    write("\n");
}

void fs_change_dir(const char *name)
{
    if (name[0] == 0)
    {
        write("Directory name cannot be empty!\n");
        return;
    }

    if (str_equal(name, ".."))
    {
        if (current_dir->parent)
        {
            current_dir = current_dir->parent;
            write("Changed current directory!\n");
        }

        return;
    }
    for (int i = 0; i < current_dir->dir_count; i++)
    {
        if (str_equal(current_dir->subdirs[i]->name, name))
        {
            current_dir = current_dir->subdirs[i];
            write("Changed current directory!\n");

            return;
        }
    }
    write("Directory not found!\n");
}

void fs_print_path(Directory *dir)
{
    if (dir->parent == (void *)0)
    {
        write("/"); // root
        return;
    }

    fs_print_path(dir->parent);

    if (dir->parent->parent != (void *)0)
        write("/");
    write(dir->name);
}

void fs_where(void)
{
    fs_print_path(current_dir);
    write("\n");
}

void fs_info(const char *name)
{
    if (name[0] == 0)
    {
        write("Directory/File name cannot be empty!\n");
        return;
    }

    // Files
    for (int i = 0; i < current_dir->file_count; i++)
    {
        if (str_equal(current_dir->files[i]->name, name))
        {
            File *f = current_dir->files[i];
            write("File: ");
            write(f->name);
            write("\nSize: ");

            char buf[12];
            int n = f->size;
            int k = 0;
            if (n == 0)
                buf[k++] = '0';
            while (n > 0)
            {
                buf[k++] = '0' + (n % 10);
                n /= 10;
            }
            for (int j = k - 1; j >= 0; j--)
                write_char(buf[j]);
            write("\n");
            return;
        }
    }

    // Directories
    for (int i = 0; i < current_dir->dir_count; i++)
    {
        if (str_equal(current_dir->subdirs[i]->name, name))
        {
            Directory *d = current_dir->subdirs[i];
            write("Directory: ");
            write(d->name);
            write("\nParent: ");
            if (d->parent)
                write(d->parent->name);
            else
                write("(root)");

            write("\nSubdirs: ");

            char buf[4];
            int n = d->dir_count;
            int k = 0;
            if (n == 0)
                buf[k++] = '0';
            while (n > 0)
            {
                buf[k++] = '0' + (n % 10);
                n /= 10;
            }
            for (int j = k - 1; j >= 0; j--)
                write_char(buf[j]);

            write("\nFiles: ");
            n = d->file_count;
            k = 0;
            if (n == 0)
                buf[k++] = '0';
            while (n > 0)
            {
                buf[k++] = '0' + (n % 10);
                n /= 10;
            }
            for (int j = k - 1; j >= 0; j--)
                write_char(buf[j]);

            write("\n");
            return;
        }
    }

    write("Directory/File not found!\n");
}

void fs_write_file(const char *name, const char *text)
{
    if (name[0] == 0)
    {
        write("File name cannot be empty!\n");
        return;
    }

    if (text[0] == 0)
    {
        write("Text cannot be empty!\n");
        return;
    }

    for (int i = 0; i < current_dir->file_count; i++)
    {
        if (str_equal(current_dir->files[i]->name, name))
        {
            File *f = current_dir->files[i];

            int j = 0;
            while (text[j] && j < sizeof(f->content) - 1)
            {
                f->content[j] = text[j];
                j++;
            }
            f->content[j] = 0;
            f->size = j;

            write("File written!\n");
            return;
        }
    }
    write("File not found!\n");
}

void fs_read_file(const char *name)
{
    if (name[0] == 0)
    {
        write("File name cannot be empty!\n");
        return;
    }

    for (int i = 0; i < current_dir->file_count; i++)
    {
        if (str_equal(current_dir->files[i]->name, name))
        {
            File *f = current_dir->files[i];
            if (f->size == 0)
            {
                write("(empty)\n");
                return;
            }
            write(f->content);
            write("\n");
            return;
        }
    }
    write("File not found!\n");
}
