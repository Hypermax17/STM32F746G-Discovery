#ifndef BSP_CPU_SENSOR_H
#define BSP_CPU_SENSOR_H

/* Interne Sensoren des STM32F746 ueber ADC1 (Kanal 18 = Temperatursensor, Kanal 17 = interne Referenz).
 * Die Funktionen liefern fertig umgerechnete physikalische Werte, die Applikation braucht keine
 * weitere Rechnung:
 *   - bsp_cpu_temperature:  Chiptemperatur (Sperrschicht) in Grad Celsius
 *   - bsp_supply_voltage:   Versorgungsspannung VDDA in Volt
 * Jeder Wert ist der Mittelwert aus 16 Messungen und wird anhand der Werkskalibrierung des Chips berechnet;
 * die Temperatur ist ausserdem um die gemessene VDDA korrigiert.
 *
 * Wichtig: Gemessen wird die Temperatur im Chip, nicht die Umgebungstemperatur. Sie liegt bei laufendem
 * Display und 216 MHz deutlich (typisch einige Grad bis > 10 K) ueber der Raumtemperatur. Der Sensor ist
 * ab Werk auf etwa +-1,5 K genau (Angabe aus dem Datenblatt des Chips).
 * Thread-sicher (Mutex). */

#define BSP_CPU_SENSOR_ERR_CAL (-3)     /* Kalibrierwerte im Chip unplausibel */

/* Rueckgabe 0 = ok */
int bsp_cpu_sensor_init(void);
int bsp_cpu_sensor_available(void);

/* Rueckgabe 0 = ok, sonst Fehler; *celsius / *volts nur bei Erfolg gesetzt */
int bsp_cpu_temperature(float *celsius);
int bsp_supply_voltage(float *volts);

#endif
