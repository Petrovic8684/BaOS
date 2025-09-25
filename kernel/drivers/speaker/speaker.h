#ifndef SPEAKER_H
#define SPEAKER_H

void speaker_init(void);
void speaker_start(unsigned int hz);
void speaker_stop(void);
void speaker_beep(unsigned int hz, unsigned int ms);

#endif
