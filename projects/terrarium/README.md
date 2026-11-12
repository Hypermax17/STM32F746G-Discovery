# Projekt Terrarium

Projektabhaengiger Teil (Branch `release/terrarium`): zusaetzliche Hardware am Board und ihre Treiber.
Die Treiber der Hardware auf dem Discovery-Board selbst liegen in `bsp/` (gemeinsam fuer alle Projekte, Branch `main`).

## Temperatur- und Feuchtesensoren DHT11 (zwei Stueck)

Treiber: `bsp/src/bsp_dht.c` (`bsp_dht.h`). Liefert fertige Werte in **Grad Celsius** und **% relative Luftfeuchte**.

### Anschluss

Der DHT11 hat keine Adresse: Jeder Sensor antwortet auf jedes Startsignal der Leitung. Zwei Sensoren an einem Pin wuerden
gleichzeitig antworten und sich stoeren. **Jeder Sensor braucht daher einen eigenen Datenpin**; Versorgung und Masse
koennen gemeinsam genutzt werden.

| Sensor | DATA (Daten) | Arduino-Anschluss | STM32 |
|---|---|---|---|
| **1** | Datenleitung | **D4** | **PG7** |
| **2** | Datenleitung | **D2** | **PG6** |
| beide | VCC (+) | 3V3 (**nicht 5 V**) | - |
| beide | GND (-) | GND | - |

Beim Makershop-Shield fuer den Wemos D1 mini heissen die drei benoetigten Anschluesse `3V3`, `G` und `D4`
(Datenleitung des Sensors); "D4" ist dort der Pin des D1 mini, nicht der des Discovery-Boards. Die Zuordnung wurde aus der
Bezeichnung des Shields uebernommen, das Datenblatt (PDF) war in der Entwicklungsumgebung nicht abrufbar.

Hinweise:
- Das Sensormodul hat normalerweise einen Pull-up-Widerstand (ca. 10 kOhm) auf der Datenleitung. Bei einem blanken
  4-Pin-DHT11 einen Widerstand 4,7..10 kOhm zwischen DATA und 3V3 setzen. Der interne Pull-up des Mikrocontrollers ist
  eingeschaltet, reicht aber bei laengeren Leitungen nicht.
- Pinzuordnung des Boards: D4 = PG7 (bestaetigt in der ST-Community), D2 = PG6 (Pinliste von Mbed OS fuer dieses Board).
  Der ST-Schaltplan wurde nicht geprueft. Beide Pins werden sonst nicht verwendet.
- Die Pins stehen in der Tabelle `s_sensor[]` in `bsp_dht.c` (Port, Pin, Name).

### Verhalten

- Messung beider Sensoren alle 2 s (`DATA_HUB_PERIOD_S`) nacheinander, der Treiber haelt den Mindestabstand von 1,1 s pro Sensor selbst ein.
- Auf dem Display (Seite Messwerte): vier Kacheln **Temp 1**, **Temp 2** (obere Zeile) und **Hum 1**, **Hum 2**
  (untere Zeile). Jede Kachel zeigt die runde Anzeige mit dem aktuellen Wert und ein Mini-Diagramm des Verlaufs (letzte
  60 Messungen, ca. 2 min) mit dem Wertebereich. Ist ein Sensor nicht erreichbar, zeigt seine Kachel `--`.
  Die Chiptemperatur (MCU) und VDDA stehen live auf der Seite System.
- Log (Seite Log): `2 x DHT11 an D4 und D2` beim Start, danach nur Aenderungen des Zustands je Sensor:
  `DHT11 1 (D4): 24 C, 55 %` bzw. eine Fehlermeldung:

| Meldung | Bedeutung |
|---|---|
| keine Antwort (Verdrahtung?) | Sensor zieht die Leitung nach dem Startsignal nicht auf Low: Anschluss, Versorgung, Pull-up pruefen |
| Signal abgebrochen | Telegramm bricht nach `n` Bit ab (Wackelkontakt, zu lange Leitung, fehlender Pull-up) |
| Pruefsumme falsch | Bitfehler (Stoerung); das Log zeigt dazu `[n Bit: xx xx xx xx xx]` = erfasste Bits und Rohbytes |
| Wert ausser Bereich | Werte ausserhalb 0..100 % rF bzw. -40..85 Grad Celsius |

- Der DHT11 misst mit 1 Grad bzw. 1 % Aufloesung; laut allgemeinem Datenblatt 0..50 Grad Celsius (+-2 K) und
  20..90 % rF (+-5 %). Die Chiptemperatur (MCU) ist **nicht** die Umgebungstemperatur.
- DHT22/AM2302: mit `-DBSP_DHT_TYPE=22` uebersetzen (Aufloesung 0,1; Skala -20..60 Grad Celsius).

Bei Fehlern wird die Messung einmal wiederholt (der Treiber wartet dafuer den Mindestabstand ab); nur ein dauerhafter
Fehler erscheint im Log. Ein Fehler mit `[40 Bit: ...]` und falscher Pruefsumme deutet auf Stoerungen oder einen zu
langsamen Pull-up, ein Abbruch mit wenigen Bits auf die Leitung oder die Versorgung.

### Messprinzip (Einzeldraht)

Der Mikrocontroller zieht die Leitung >= 18 ms nach Masse und laesst los; die Leitung steigt dann ueber den Pull-up
an (Anstiegszeit, je nach Widerstand und Leitung einige us). Erst danach antwortet der Sensor mit 80 us Low und 80 us High,
gefolgt von 40 Bits (je 50 us Low, dann High 26 us = 0 bzw. 70 us = 1). Gemessen wird mit dem Zyklenzaehler des
Prozessors (216 Schritte pro us); waehrend des ca. 4 ms langen Telegramms sind Interrupts mit FreeRTOS-Prioritaet gesperrt.
Die Sensoren werden nacheinander gelesen, nie gleichzeitig.

### Tests

`tests/host/run.sh` simuliert den Sensor (Einzeldraht-Protokoll mit virtueller Zeit, Zeitjitter, Fehlerfaelle) und prueft
Erfassung und Umrechnung. Das Zeitverhalten auf echter Hardware (Pin, Pull-up, Leitung) ist damit nicht abgedeckt.
