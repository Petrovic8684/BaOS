#include "./melodies.h"
#include "../speaker.h"

void play_startup_melody(void)
{
    __asm__ volatile("sti");
    speaker_beep(220, 100);
    speaker_beep(260, 100);
    speaker_beep(300, 100);
    speaker_beep(330, 150);
    speaker_beep(370, 200);
    __asm__ volatile("cli");
}

void play_shutdown_melody(void)
{
    __asm__ volatile("sti");
    speaker_beep(370, 100);
    speaker_beep(330, 100);
    speaker_beep(300, 100);
    speaker_beep(260, 150);
    speaker_beep(220, 200);
    __asm__ volatile("cli");
}

void play_restart_melody(void)
{
    __asm__ volatile("sti");
    speaker_beep(300, 120);
    speaker_beep(370, 120);
    speaker_beep(440, 150);
    speaker_beep(370, 120);
    speaker_beep(300, 150);
    __asm__ volatile("cli");
}