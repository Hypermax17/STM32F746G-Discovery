#ifndef BSP_SENSOR_MATH_H
#define BSP_SENSOR_MATH_H

#include <stdint.h>

/* Umrechnung der ADC-Rohwerte in physikalische Groessen (reine Rechnung, ohne Hardwarezugriff,
 * daher auf dem PC testbar). Kalibrierwerte stammen aus dem Flash des Chips (Werkskalibrierung bei
 * VDDA = 3,3 V; Temperaturen 30 und 110 Grad Celsius). */

#define BSP_SENSOR_CAL_VDDA_V     3.3f
#define BSP_SENSOR_CAL1_TEMP_C    30.0f
#define BSP_SENSOR_CAL2_TEMP_C    110.0f

/* VDDA in Volt aus dem Rohwert der internen Referenz (VREFINT liegt fest bei ca. 1,21 V). */
static inline float bsp_sensor_vdda_volts(float vref_raw, uint16_t vref_cal)
{
    return BSP_SENSOR_CAL_VDDA_V * (float)vref_cal / vref_raw;
}

/* Chiptemperatur in Grad Celsius. Der Rohwert wird zuerst auf die Kalibrierspannung (3,3 V) umgerechnet,
 * weil sich der ADC-Wert mit VDDA verschiebt; danach lineare Interpolation zwischen den beiden Werkspunkten. */
static inline float bsp_sensor_temp_celsius(float ts_raw, float vdda_v, uint16_t cal1, uint16_t cal2)
{
    const float ts_at_cal_vdda = ts_raw * vdda_v / BSP_SENSOR_CAL_VDDA_V;
    return BSP_SENSOR_CAL1_TEMP_C +
           (BSP_SENSOR_CAL2_TEMP_C - BSP_SENSOR_CAL1_TEMP_C) * (ts_at_cal_vdda - (float)cal1) /
               (float)((int32_t)cal2 - (int32_t)cal1);
}

#endif
