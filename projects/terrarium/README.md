# Projekt Terrarium

Projektabhaengiger Teil (Branch `release/terrarium`): zusaetzliche Hardware am Board und ihre Treiber.
Die Treiber der Hardware auf dem Discovery-Board selbst liegen in `bsp/` (gemeinsam fuer alle Projekte, Branch `main`).

## Temperatur- und Feuchtesensor DHT11

Treiber: `bsp/src/bsp_dht.c` (`bsp_dht.h`). Liefert fertige Werte in **Grad Celsius** und **% relative Luftfeuchte**.

### Anschluss

| Sensor | Arduino-Anschluss des Boards | STM32 |
|---|---|---|
| **DATA** (Daten) | **D4** | **PG7** |
| VCC (+) | 3V3 (**nicht 5 V**) | - |
| GND (-) | GND | - |

Beim Makershop-Shield fuer den Wemos D1 mini heissen die drei benoetigten Anschluesse `3V3`, `G` und `D4`
(Datenleitung des Sensors); "D4" ist dort der Pin des D1 mini, nicht der des Discovery-Boards. Die Zuordnung wurde aus der
Bezeichnung des Shields uebernommen, das Datenblatt (PDF) war in der Entwicklungsumgebung nicht abrufbar: bitte die
Beschriftung auf dem Shield pruefen.

Hinweise:
- Das Sensormodul hat normalerweise einen Pull-up-Widerstand (ca. 10 kOhm) auf der Datenleitung. Bei einem blanken
  4-Pin-DHT11 einen Widerstand 4,7..10 kOhm zwischen DATA und 3V3 setzen. Der interne Pull-up des Mikrocontrollers ist
  eingeschaltet, reicht aber bei laengeren Leitungen nicht.
- Der Pin PG7 wird auf dem Board sonst nicht verwendet (laut ST-Community liegt PG7 am Arduino-Pin D4;
  bei Revisionen des Boards sind nur D5 (PI0) und D10 (PA8) vertauscht).
- Der Pin ist in `bsp_dht.c` festgelegt (`DHT_PORT`, `DHT_PIN`).

### Verhalten

- Messung alle 2 s (`DATA_HUB_PERIOD_S`), der Treiber haelt den Mindestabstand von 1,1 s selbst ein.
- Auf dem Display (Seite Messwerte): Anzeigen **MCU**, **Hum**, **Temp**; Diagramm und Tabelle zeichnen alle drei Werte
  auf. Ist kein Sensor erreichbar, zeigen Hum und Temp `--`, die MCU-Temperatur laeuft weiter.
- Log (Seite Log): `DHT11 an PG7 (Arduino D4)` beim Start, danach nur Aenderungen des Zustands:
  `DHT11: 24 C, 55 %` bzw. eine Fehlermeldung:

| Meldung | Bedeutung |
|---|---|
| keine Antwort (Verdrahtung?) | Sensor zieht die Leitung nach dem Startsignal nicht auf Low: Anschluss, Versorgung, Pull-up pruefen |
| Signal abgebrochen | Telegramm bricht ab (Wackelkontakt, zu lange Leitung) |
| Pruefsumme falsch | Bitfehler (Stoerung) |
| Wert ausser Bereich | Werte ausserhalb 0..100 % rF bzw. -40..85 Grad Celsius |

- Der DHT11 misst mit 1 Grad bzw. 1 % Aufloesung; laut allgemeinem Datenblatt 0..50 Grad Celsius (+-2 K) und
  20..90 % rF (+-5 %). Die Chiptemperatur (MCU) ist **nicht** die Umgebungstemperatur.
- DHT22/AM2302: mit `-DBSP_DHT_TYPE=22` uebersetzen (Aufloesung 0,1; Skala -20..60 Grad Celsius).

### Tests

`tests/host/run.sh` simuliert den Sensor (Einzeldraht-Protokoll mit virtueller Zeit, Zeitjitter, Fehlerfaelle) und prueft
Erfassung und Umrechnung. Das Zeitverhalten auf echter Hardware (Pin, Pull-up, Leitung) ist damit nicht abgedeckt.
