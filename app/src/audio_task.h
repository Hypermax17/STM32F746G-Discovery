#ifndef AUDIO_TASK_H
#define AUDIO_TASK_H

/* Startet die Audio-Ausgabe: initialisiert den Codec, spielt beim Start einen kurzen
 * 440-Hz-Ton und erzeugt danach einen Sinuston nach Beruehrung des Displays
 * (X-Position = Tonhoehe 200..2000 Hz, Y-Position = Lautstaerke, oben laut). */
void audio_task_start(void);

/* Von der Touch-Auswertung aufgerufen: aktuelle Beruehrung (active = 0: keine). */
void audio_task_set_touch(int active, unsigned x, unsigned y);

#endif
