#ifndef FAKE_SEMPHR_H
#define FAKE_SEMPHR_H
#include "FreeRTOS.h"
static inline SemaphoreHandle_t xSemaphoreCreateMutex(void) { return (void *)1; }
#define xSemaphoreTake(m, t) ((void)0)
#define xSemaphoreGive(m) ((void)0)
#endif
