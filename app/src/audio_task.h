#ifndef AUDIO_TASK_H
#define AUDIO_TASK_H

/* Startet die Audio-Ausgabe: initialisiert den Codec und spielt beim Start einen kurzen 440-Hz-Ton. */
void audio_task_start(void);

/* Test-Ton (440 Hz) ein-/ausschalten, z. B. waehrend ein Button gedrueckt ist. */
void audio_task_beep(int on);

#endif
