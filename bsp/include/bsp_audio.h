#ifndef BSP_AUDIO_H
#define BSP_AUDIO_H

#include <stdint.h>

/* Audio-Ausgabe: Codec WM8994 (I2C3, 7-Bit-Adresse 0x1A) am Kopfhoerer-Ausgang, angebunden ueber
 * SAI2 Block A (I2S, 16 Bit, Stereo, 48 kHz, MCLK 256*fs vom STM32) mit DMA im Ringpuffer.
 *   SAI2: MCLK = PI4, SCK = PI5, SD = PI6, FS = PI7 (AF10)
 *
 * Ablauf:  bsp_audio_init() -> bsp_audio_start() -> in einer Schleife
 *          bsp_audio_next_buffer() / Puffer fuellen / bsp_audio_release_buffer().
 * Der Ringpuffer besteht aus zwei Haelften; sobald der DMA eine Haelfte abgespielt hat, wird sie
 * freigegeben und kann neu gefuellt werden (je BSP_AUDIO_HALF_FRAMES Frames = 10 ms). */

#define BSP_AUDIO_SAMPLE_RATE   48000U
#define BSP_AUDIO_HALF_FRAMES   480U     /* Frames pro Pufferhaelfte (1 Frame = links + rechts) */
#define BSP_AUDIO_DEFAULT_VOLUME 70U     /* Prozent */

/* Taktet SAI2, initialisiert den Codec (Kopfhoerer). Nutzt vTaskDelay, wenn der Scheduler laeuft
 * (ca. 0,4 s). Rueckgabe 0 = ok; -1 = Codec antwortet nicht / falsche ID, -2 = SAI/DMA-Fehler.
 * *codec_id erhaelt die gelesene Chip-ID (erwartet 0x8994). */
int bsp_audio_init(uint16_t *codec_id);

/* 1, wenn bsp_audio_init() erfolgreich war */
int bsp_audio_is_ready(void);

/* Beginnt die Ausgabe (zunaechst Stille) und hebt die Stummschaltung auf. */
int bsp_audio_start(void);
void bsp_audio_stop(void);

/* Wartet auf eine freie Pufferhaelfte: *buf zeigt auf BSP_AUDIO_HALF_FRAMES * 2 Samples
 * (int16, interleaved L R). Rueckgabe 0 = ok, -1 = Timeout. */
int bsp_audio_next_buffer(int16_t **buf, uint32_t timeout_ms);

/* Gibt die gefuellte Haelfte zur Wiedergabe frei (schreibt den Cache zurueck). */
void bsp_audio_release_buffer(int16_t *buf);

/* Lautstaerke 0..100 % (Kopfhoerer-Verstaerker; 100 % = 0 dB), Stummschaltung */
int bsp_audio_set_volume(uint8_t percent);
int bsp_audio_set_mute(int mute);

/* Diagnose: gibt SAI-/DMA-Zustand (nach 1 s Messung) und wichtige Codec-Register im Log aus.
 * Nur aus einer Task aufrufbar. */
void bsp_audio_dump(void);

/* Zaehler fuer verpasste Pufferhaelften (Task war zu langsam) und SAI-Fehler */
uint32_t bsp_audio_underruns(void);
uint32_t bsp_audio_errors(void);

#endif
