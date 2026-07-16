#include "./system/pic/pic.h"
#include "./system/idt/idt.h"
#include "./drivers/drivers.h"
#include "./drivers/pit/pit.h"
#include "./drivers/speaker/melodies/melodies.h"
#include "./segmentation/segmentation.h"
#include "./segmentation/heap/heap.h"
#include "./system/tss/tss.h"
#include "./fs/fs.h"
#include "./loader/loader.h"

#define LOG_DELAY 300

__attribute__((section(".text"), used, noreturn)) void kernel_main(void)
{
    pic_init();
    idt_init();
    drivers_init(LOG_DELAY);

    segmentation_init();
    pit_sleep(LOG_DELAY);
    heap_init();
    pit_sleep(LOG_DELAY);

    tss_init();
    pit_sleep(LOG_DELAY);

    fs_init();
    play_startup_melody();

    load_welcome();
    load_shell();

    while (1)
        ;
}
