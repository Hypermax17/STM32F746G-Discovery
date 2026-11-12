/* PC-Tests fuer reine Rechenfunktionen: Sensor-Umrechnung und Zahlenformatierung. */
#include "bsp_sensor_math.h"
#include "bsp_util.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK_NEAR(actual, expected, tol, what) do { \
    float a_ = (actual), e_ = (expected); \
    if (fabsf(a_ - e_) > (tol)) { printf("FEHLER %s: %f statt %f\n", what, a_, e_); failures++; } \
    else printf("ok     %s = %f\n", what, a_); } while (0)

static void check_fmt(float v, unsigned dec, const char *expected)
{
    char b[32];
    bsp_format_float(b, sizeof b, v, dec);
    if (strcmp(b, expected) != 0) { printf("FEHLER format(%f,%u) = \"%s\" statt \"%s\"\n", v, dec, b, expected); failures++; }
    else printf("ok     format(%f,%u) = \"%s\"\n", v, dec, b);
}

int main(void)
{
    /* Beispiel-Kalibrierwerte (wie vom Werk: TS_CAL1 bei 30 C, TS_CAL2 bei 110 C, bei VDDA = 3,3 V) */
    const uint16_t cal1 = 0x2A0, cal2 = 0x3C0, vref_cal = 0x5F0;

    CHECK_NEAR(bsp_sensor_temp_celsius(cal1, 3.3f, cal1, cal2), 30.0f, 0.001f, "T bei CAL1");
    CHECK_NEAR(bsp_sensor_temp_celsius(cal2, 3.3f, cal1, cal2), 110.0f, 0.001f, "T bei CAL2");
    CHECK_NEAR(bsp_sensor_temp_celsius((cal1 + cal2) / 2.0f, 3.3f, cal1, cal2), 70.0f, 0.001f, "T Mitte");
    /* Bei 3,0 V Versorgung steigt der Rohwert um 3,3/3,0; mit Korrektur muss wieder 30 C herauskommen */
    CHECK_NEAR(bsp_sensor_temp_celsius(cal1 * 3.3f / 3.0f, 3.0f, cal1, cal2), 30.0f, 0.01f, "T bei VDDA 3,0 V");

    CHECK_NEAR(bsp_sensor_vdda_volts(vref_cal, vref_cal), 3.3f, 0.0001f, "VDDA kalibriert");
    CHECK_NEAR(bsp_sensor_vdda_volts(vref_cal * 3.3f / 3.0f, vref_cal), 3.0f, 0.001f, "VDDA 3,0 V");

    check_fmt(41.3f, 1, "41.3");
    check_fmt(41.26f, 1, "41.3");
    check_fmt(-12.5f, 1, "-12.5");
    check_fmt(-0.04f, 1, "0.0");
    check_fmt(3.31f, 2, "3.31");
    check_fmt(100.0f, 0, "100");
    check_fmt(0.05f, 3, "0.050");
    check_fmt(7.0f, 2, "7.00");

    printf(failures ? "\n%d FEHLER\n" : "\nalle Tests bestanden\n", failures);
    return failures ? 1 : 0;
}
