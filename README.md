# STM32F746G-Discovery

Basisapplikation fuer das STM32F746G-Discovery-Board: **BSP** (Treiber) und **Applikation** getrennt,
FreeRTOS, Build mit CMake + `arm-none-eabi-gcc`. Debug-Ausgaben erscheinen auf dem **Display** (kein UART).

## Struktur

| Ordner | Inhalt |
|---|---|
| `bsp/` | Board-Support-Package: ein Treiber je Peripherie (`include/` = API, `src/` = Implementierung, `config/` = HAL-Konfiguration) |
| `app/` | Applikation (Tasks, Logik) und `config/FreeRTOSConfig.h`. Greift nur ueber das BSP auf Hardware zu |
| `third_party/` | Git-Submodule: STM32F7-HAL, CMSIS-Core/-Device, FreeRTOS-Kernel |
| `linker/` | Linker-Script (Flash, DTCM, SRAM, SDRAM) |
| `cmake/` | Toolchain-Datei |
| `docs/` | Doku (Architektur, Flashen, optionale Module) |

## Bauen

Voraussetzungen: `arm-none-eabi-gcc` (>= 10), `cmake` (>= 3.21), `ninja`.

```bash
git clone --recurse-submodules <repo-url>     # oder: git submodule update --init
cmake --preset release
cmake --build --preset release
```

Ergebnis in `build/release/app/`: `stm32f746g_app.bin`, `.hex`, `.elf`, `.map`.
Liegt der Compiler nicht im `PATH`, die Umgebungsvariable `ARM_TOOLCHAIN_PATH` auf das `bin`-Verzeichnis setzen.
Bei jedem Push baut GitHub Actions (`.github/workflows/build.yml`) die Applikation (`scripts/build_variants.sh`) und stellt sie als **ein** Artefakt `stm32f746g-hex` mit der `.hex`-Datei bereit.

## Bedienung der Basisapplikation

Oben im Display steht eine Tab-Leiste. Antippen oder ein kurzer Druck auf den User-Taster (B1) wechselt die Seite:

| Seite | Inhalt |
|---|---|
| **SYSTEM** | Peripherie-Status (Display, SDRAM, Touch, QSPI, Audio, Taster; SD/USB/ETH/CAM = Geruest), Laufzeit, freier Heap, CPU-Last und Tasks (Prioritaet, freier Stack, CPU-Anteil) |
| **TOUCH** | Zeichenflaeche fuer bis zu 5 Finger (Farbe je Finger); gleichzeitig Sinuston: X = Tonhoehe, Y = Lautstaerke. Erneutes Tippen auf den Tab TOUCH loescht die Zeichnung |
| **LOG** | Meldungen des Systems (die Boot-Meldungen stehen hier) |

Die Systemseite zeigt auch die Chiptemperatur (Grad Celsius) und die Versorgungsspannung (Volt), siehe `docs/architektur.md` (Abschnitt Sensorik).

Weitere Tasterfunktionen: 1..3 s halten = Audio-Diagnose im Log, >= 3 s = QSPI-Schreibtest.

## Flashen (STM32CubeProgrammer)

Siehe [docs/flashen.md](docs/flashen.md).

## Stand

- [x] Schritt 0: Projektgeruest, Pipeline, FreeRTOS, Takt 216 MHz
- [x] Schritt 1: SDRAM, LCD, Debug-Konsole auf dem Display
- [x] Schritt 2: Fehlerbehandlung (Hard-Fault etc.) auf dem Display
- [x] Schritt 3: Touchscreen
- [x] Schritt 4: User-Button, LED
- [x] Schritt 5: QSPI-Flash
- [x] Schritt 6: Audio
- [x] Schritt 7: Geruest fuer SD, USB OTG, Ethernet, Kamera (`docs/optionale-module.md`)
- [x] Schritt 8: Basisapplikation (Seiten SYSTEM, TOUCH, LOG)
