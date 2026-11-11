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
| LCD | LTDC-Datenleitungen auf PE4, PG12, PI9/10/14/15, PJ0..11/13..15, PK0..2/4..7; DISP = PI12; Backlight = PK3 |
| SDRAM (FMC) | PC3, PD0/1/8/9/10/14/15, PE0/1/7..15, PF0..5/11..15, PG0/1/4/5/8/15, PH3/5 |
