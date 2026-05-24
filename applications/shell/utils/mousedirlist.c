#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <baos/mouse.h>
#include <baos/vga.h>
#include "dirlist_common.h"

#define MAX_ENTRIES 32

static dir_entry_t entries[MAX_ENTRIES];
static int entry_count;
static char base_path[256];

static int mc_row = -1;
static int mc_col = -1;
static char mc_ch = ' ';
static unsigned char mc_attr = 0x07;
static int mc_visible;

static int entry_index_at(int row, int col)
{
    for (int i = 0; i < entry_count; i++)
    {
        dir_entry_t *e = &entries[i];
        if (row == e->row && col >= e->col && col < e->col + e->len)
            return i;
    }
    return -1;
}

static void mouse_cursor_hide(void)
{
    if (!mc_visible)
        return;

    vga_put_cell(mc_row, mc_col, mc_ch, mc_attr);
    mc_visible = 0;
}

static void mouse_cursor_show(int row, int col)
{
    if (mc_visible && row == mc_row && col == mc_col)
        return;

    mouse_cursor_hide();

    mc_row = row;
    mc_col = col;
    vga_get_cell(row, col, &mc_ch, &mc_attr);
    vga_put_cell(row, col, mc_ch, vga_invert_attr(mc_attr));
    mc_visible = 1;
}

static void restore_text_cursor(void)
{
    mouse_cursor_hide();
    printf("\033[?25h");
}

/* 0 = success, 1 = error exit */
static int handle_left_click(int row, int col)
{
    restore_text_cursor();

    int idx = entry_index_at(row, col);
    if (idx < 0)
    {
        printf("\033[31mError: No directory at cursor.\033[0m\n");
        return 1;
    }

    dir_entry_t *e = &entries[idx];
    if (!e->is_dir)
    {
        printf("\033[31mError: '%s' is not a directory.\033[0m\n", e->name);
        return 1;
    }

    char target[256];
    dirlist_join_path(target, sizeof(target), base_path, e->name);

    if (dirlist_print_contents(target, stdout) != 0)
    {
        printf("\033[31mError: Could not open directory '%s'.\033[0m\n", target);
        return 1;
    }

    dirlist_save_context(target);
    return 0;
}

static int should_exit(const mouse_event_t *ev)
{
    return ev->type == MOUSE_EV_BUTTON && ev->button.button == 2 && ev->button.pressed;
}

int main(int argc, char *argv[])
{
    char raw_path[256];
    memset(raw_path, 0, sizeof(raw_path));

    if (argc > 1 && argv[1][0] != '\0')
    {
        strncpy(raw_path, argv[1], sizeof(raw_path) - 1);
    }
    else if (dirlist_load_context(raw_path, sizeof(raw_path)) != 0)
    {
        strncpy(raw_path, ".", sizeof(raw_path) - 1);
    }

    if (dirlist_resolve_path(raw_path, base_path, sizeof(base_path)) != 0)
    {
        printf("\033[31mError: Could not resolve directory path.\033[0m\n");
        return 1;
    }

    DIR *verify = opendir(base_path);
    if (!verify)
    {
        dirlist_clear_context();
        printf("\033[31mError: Listing path '%s' is not accessible. Run 'dirlist' first.\033[0m\n", base_path);
        return 1;
    }
    closedir(verify);

    entry_count = dirlist_scan_listing_entries(base_path, entries, MAX_ENTRIES);
    if (entry_count <= 0)
    {
        printf("\033[31mError: No dirlist output found on screen. Run 'dirlist' first.\033[0m\n");
        return 1;
    }

    printf("\033[?25l");

    mouse_event_t ev;
    while (mouse_peek(&ev))
        mouse_read(&ev);

    int mx = 0, my = 0;
    mouse_getpos(&mx, &my);
    mouse_cursor_show(my, mx);

    while (1)
    {
        mouse_getpos(&mx, &my);
        mouse_cursor_show(my, mx);

        if (mouse_peek(&ev))
        {
            if (mouse_read(&ev))
            {
                if (should_exit(&ev))
                    break;

                if (ev.type == MOUSE_EV_BUTTON && ev.button.button == 1 && ev.button.pressed)
                    return handle_left_click(ev.button.y, ev.button.x);
            }
        }

        usleep(5000);
    }

    restore_text_cursor();
    return 0;
}
