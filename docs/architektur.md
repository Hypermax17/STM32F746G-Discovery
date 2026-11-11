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
| SDRAM | `0xC0000000`, 8 MB | `0xC0000000`: Framebuffer (RGB565, 255 KB, 512 KB reserviert); ab `0xC0080000` frei (`.sdram`) |

Hinweis: Der SDRAM-Chip hat 128 Mbit (16 MB), ist am STM32F746 aber nur mit 16 Datenleitungen angebunden, daher sind **8 MB** nutzbar.

Der SDRAM-Bereich ist per MPU als *Normal, cachebar, write-through* konfiguriert (`bsp_system.c`),
damit der LTDC (DMA) immer aktuelle Daten liest und unaligned Zugriffe erlaubt sind.

## Takte

HSE 25 MHz -> PLL -> SYSCLK/HCLK 216 MHz, APB1 54 MHz, APB2 108 MHz (Over-Drive, Flash 7 WS).
LTDC-Pixeltakt 9,6 MHz aus PLLSAI (N=192, R=5, DivR=4).
Wird spaeter Audio (SAI) ergaenzt, muss die PLLSAI-Konfiguration beide Verbraucher beruecksichtigen.

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
