/* Minimale FreeRTOS-Attrappen, damit bsp_console.c auf dem PC uebersetzt werden kann */
#ifndef FAKE_FREERTOS_H
#define FAKE_FREERTOS_H
#include <stdint.h>
#include <stddef.h>
#define portMAX_DELAY 0xFFFFFFFFu
typedef void *SemaphoreHandle_t;
#endif
