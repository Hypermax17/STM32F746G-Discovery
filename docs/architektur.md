# Architektur

## Schichten

```
app/   Applikation (FreeRTOS-Tasks, Logik)      -> nutzt nur bsp.h
bsp/   Treiber je Peripherie (bsp_<name>.c/.h)  -> nutzt HAL + FreeRTOS
third_party/  HAL, CMSIS, FreeRTOS
```

`bsp_init()` bringt das Board in dieser Reihenfolge hoch:
HAL-Init -> MPU/Caches -> LED -> Takt 216 MHz -> SDRAM (+ Test) -> LCD -> Konsole.
Schlaegt ein Schritt vor der Konsole fehl, zeigt LD1 einen Blinkcode.

## Speicherkarte

| Bereich | Adresse | Nutzung |
|---|---|---|
| Flash | `0x08000000`, 1 MB | Programm |
| DTCM | `0x20000000`, 64 KB | Main-Stack (MSP) |
| SRAM1+2 | `0x20010000`, 256 KB | `.data`/`.bss`, FreeRTOS-Heap (96 KB) |
| QSPI-Flash | `0x90000000`, 16 MB | Memory-Mapped (nur lesbar), per MPU gesperrt solange das Mapping aus ist |
| SDRAM | `0xC0000000`, 8 MB | `0xC0000000`: Framebuffer (RGB565, 255 KB, 512 KB reserviert); ab `0xC0080000` frei (`.sdram`) |

Hinweis: Der SDRAM-Chip hat 128 Mbit (16 MB), ist am STM32F746 aber nur mit 16 Datenleitungen angebunden, daher sind **8 MB** nutzbar.

Der SDRAM-Bereich ist per MPU als *Normal, cachebar, write-through* konfiguriert (`bsp_system.c`),
damit der LTDC (DMA) immer aktuelle Daten liest und unaligned Zugriffe erlaubt sind.

## FPU

Der STM32F746 hat eine **Single-Precision**-FPU (`-mfpu=fpv5-sp-d16`). Berechnungen mit `float` laufen in Hardware,
`double` wird per Software emuliert (langsam). Mit `-mfpu=fpv5-d16` kaeme es zu einem UsageFault
"Undefinierter Befehl" bei der ersten Double-Operation (so geschehen in `sinf()` des Audio-Tasks).
Im Code daher `float` und `sinf/cosf` statt `double` verwenden.

## Takte

HSE 25 MHz -> PLL -> SYSCLK/HCLK 216 MHz, APB1 54 MHz, APB2 108 MHz (Over-Drive, Flash 7 WS).
LTDC-Pixeltakt 9,6 MHz aus PLLSAI (N=192, R=5, DivR=4).
Der Audio-Takt kommt aus einer eigenen PLL (PLLI2S, siehe Abschnitt Audio), beide PLLs sind unabhaengig.

## Interrupts / FreeRTOS

- FreeRTOS nutzt SysTick; die HAL-Zeitbasis (`HAL_GetTick`) laeuft auf **TIM6** (Prioritaet 1).
- Prioritaetsgruppen: 4 Bit Preemption. Interrupts, die FreeRTOS-`...FromISR`-Funktionen aufrufen, brauchen Prioritaet 5..15.

## Display-Konsole

`bsp_console` (40x17 Zeichen, 5x7-Font skaliert x2): 16 scrollende Logzeilen + 1 Statuszeile.
`BSP_LOGD/I/W/E()` aus `bsp_log.h` (printf-Format, ohne `%f`). `bsp_panic()` zeigt Fehler in Rot und blinkt LD1 (6x).

## Pinbelegung (verwendet)

| Funktion | Pins |
|---|---|
| LED LD1 | PI1 |
| User-Taster B1 | PI11 (aktiv high, EXTI15_10) |
| LCD | LTDC-Datenleitungen auf PE4, PG12, PI9/10/14/15, PJ0..11/13..15, PK0..2/4..7; DISP = PI12; Backlight = PK3 |
| Audio (SAI2, Codec WM8994) | PI4 = MCLK, PI5 = SCK, PI6 = SD, PI7 = FS (AF10); Steuerung ueber I2C3 |
| QSPI-Flash (QUADSPI) | PB2 = CLK (AF9), PB6 = NCS (AF10), PD11 = IO0, PD12 = IO1, PE2 = IO2, PD13 = IO3 (AF9) |
| I2C3 (Touch, Audio-Codec) | PH7 = SCL, PH8 = SDA (AF4, Open-Drain); Touch-Interrupt PI13 (noch ungenutzt) |
| SDRAM (FMC) | PC3, PD0/1/8/9/10/14/15, PE0/1/7..15, PF0..5/11..15, PG0/1/4/5/8/15, PH3/5 |

## Fehlerbehandlung (`bsp_fault.c`)

NMI, HardFault, MemManage, BusFault und UsageFault loeschen den Logbereich und zeigen:
Fault-Art, Task, PC/LR/PSR/SP, R0-R3/R12, CFSR/HFSR/MMFAR/BFAR und die dekodierte Ursache.
Danach blinkt LD1 7x; neu starten mit dem Reset-Taster. Der Handler nutzt kein printf.

Aktiv sind ausserdem: Division-durch-0-Trap, eine MPU-Falle auf Adresse 0..31 (Null-Zeiger),
keine Code-Ausfuehrung im SDRAM, FreeRTOS-Stack-Ueberlauf-Pruefung (Methode 2), Heap-Fehler-Hook und `configASSERT`
(Ausgabe per `bsp_panic()`).

### Fehlerdarstellung testen

Mit einem Konfigurations-Schalter loest die App 3 s nach dem Start gezielt einen Fehler aus:

```bash
cmake --preset release -DAPP_FAULT_TEST=nullptr
cmake --build --preset release
```

Arten: `nullptr`, `div0`, `undef`, `execsdram`, `stackoverflow`, `assert`, `malloc`.
Zurueck zum Normalbetrieb: `cmake --preset release -DAPP_FAULT_TEST=` und neu bauen.
Die CI baut diese Varianten nicht; lokal: `FAULT_TESTS="nullptr div0" scripts/build_variants.sh`.

## I2C3 und Touchscreen

`bsp_i2c` (I2C3, 100 kHz, Kerntakt SYSCLK, Mutex) ist der gemeinsame Bus von Touch-Controller
FT5336 (7-Bit `0x38`) und spaeter dem Audio-Codec WM8994 (7-Bit `0x1A`).
`bsp_touch` liest per Polling (20 ms) bis zu 5 Beruehrungspunkte und bildet sie auf Display-Koordinaten
(0..479 / 0..271) ab. Die Ausrichtung ist ueber `BSP_TOUCH_DEFAULT_SWAP_XY/INVERT_X/INVERT_Y`
(`bsp_touch.h`) bzw. `bsp_touch_set_orientation()` einstellbar.

Die App (`touch_task.c`) loggt Beruehrungen mit Display- und Rohkoordinaten ("Touch(n) x,y raw rx,ry")
und zeigt die aktuelle Position rechts in der Statuszeile.

## User-Taster und LED

`bsp_button`: Flanken an PI11 loesen EXTI15_10 aus (Prioritaet 6, FreeRTOS-API erlaubt). Die ISR entprellt
(20 ms, Pegelwechsel gegenueber dem zuletzt akzeptierten Zustand) und legt `PRESSED`/`RELEASED`-Ereignisse mit
Zeitstempel in eine Queue; Tasks lesen sie mit `bsp_button_get_event()`.

App: `button_task` loggt kurze und lange Druecke (>= 1 s). Ein kurzer Druck wechselt das Blinktempo von LD1
(500 ms <-> 100 ms), das der `heartbeat_task` ausfuehrt.

## QSPI-Flash

`bsp_qspi`: Micron MT25QL128 (16 MB, JEDEC-ID `20 BA 18`) am QUADSPI, 108 MHz, Standard-SPI-Protokoll,
Lesen per *Quad Output Fast Read* (`0x6B`, 8 Dummy-Takte), Schreiben per Page Program (`0x02`, 256 B),
Loeschen 4 KB (`0x20`) bzw. 64 KB (`0xD8`). Beim Init wird der Chip zurueckgesetzt (auch aus dem QPI-Modus,
falls ein ST-Demo ihn dort gelassen hat), die ID geprueft und das **Memory-Mapping** ab `0x90000000` eingeschaltet.

- Indirekt: `bsp_qspi_read/write/erase_subsector/erase_sector` (Adresse = Offset). Schreiben/Loeschen schalten das
  Mapping kurz ab und danach wieder an; Bereiche muessen vorher geloescht (0xFF) sein. Alles Thread-sicher.
- Memory-mapped: Der Flash ist ab `BSP_QSPI_BASE` wie RAM lesbar (Cache: write-through, nur lesen).
- MPU-Region 2 sperrt `0x90000000..0x90FFFFFF`, solange das Mapping aus ist. Grund: Der Cortex-M7 darf nicht
  spekulativ auf den abgeschalteten QUADSPI zugreifen (Bus-Haenger).

Beim Boot wird nichts geschrieben oder geloescht. Das Log zeigt `QSPI: 16 MB ID 20 BA 18` und
`QSPI: Mapping 0x90000000 ok` (64 Bytes werden indirekt und per Mapping verglichen).

### Schreibtest (Taster)

Ein Tastendruck von **>= 3 s** startet `qspi_write_test()`: Es wird von hinten ein komplett leerer 4-KB-Sektor
gesucht (vorhandene Daten, z. B. vom ST-Demo, bleiben unangetastet), ein Muster programmiert, indirekt und per
Mapping geprueft und der Sektor wieder geloescht. Das Log zeigt Zeiten und Fehleranzahl.

## Audio

`bsp_audio` gibt Stereo-Ton (16 Bit, 48 kHz) ueber den Kopfhoerer-Ausgang des Audio-Codecs WM8994 aus.

- **Takt:** PLLI2S (VCO 344 MHz, /7 = 49,14 MHz) speist SAI2; das HAL teilt daraus MCLK = 256 * fs (12,29 MHz).
  Der STM32 ist I2S-Master (SAI2 Block A: MCLK, SCK, FS, SD), der Codec Slave.
- **Codec** (I2C3, 7-Bit `0x1A`, Register 16 Bit): Chip-ID (`0x8994`) pruefen, DAC1 -> Kopfhoerer-Pfad,
  Kopfhoerer-Kaltstart ueber die eingebaute Write-Sequenz (Register `0x110`, ca. 300 ms), Lautstaerke ueber
  `0x1C/0x1D`, Stummschaltung ueber `0x420/0x422`.
- **Datenfluss:** DMA2 Stream 4 im Ringpuffer (2 Haelften zu je 480 Frames = 10 ms). Fertig abgespielte Haelften
  melden DMA-Interrupts ueber eine Queue; der Audio-Task holt sie mit `bsp_audio_next_buffer()`, fuellt sie und gibt
  sie mit `bsp_audio_release_buffer()` frei (Cache-Clean fuer den DMA).
- **App:** `audio_task` spielt beim Start 300 ms 440 Hz. Danach erzeugt eine Beruehrung einen Sinuston:
  X-Position = Tonhoehe (200..2000 Hz), Y-Position = Lautstaerke (oben laut). Loslassen blendet aus.
  Standard-Lautstaerke 70 % (Kopfhoerer-Verstaerker), maximaler Pegel ca. 37 % Vollaussteuerung.
- **Nur Ausgabe:** Eingaenge (Mikrofone, Line-In) sind noch nicht implementiert.
