#!/usr/bin/env bash
# Rendert die Display-Konsole auf dem PC (ohne Board) und schreibt PNGs nach $OUT (Standard: build/host-render).
# Benutzt dieselben Quelldateien wie die Firmware (bsp_console, bsp_gfx, bsp_font).
set -euo pipefail
cd "$(dirname "$0")/../.."
export OUT="${OUT:-build/host-render}"
mkdir -p "$OUT"
gcc -std=gnu11 -Wall -Wextra -o "$OUT/render_test" \
    -I tests/host/stubs -I bsp/include \
    tests/host/render_test.c tests/host/fake_lcd.c \
    bsp/src/bsp_console.c bsp/src/bsp_gfx.c bsp/src/bsp_font.c
"$OUT/render_test"
python3 tests/host/topng.py

# Rechentests (Sensor-Umrechnung, Zahlenformatierung)
gcc -std=gnu11 -Wall -Wextra -o "$OUT/unit_test" -I bsp/include tests/host/unit_test.c bsp/src/bsp_util.c -lm
"$OUT/unit_test"

# DHT-Treiberkern (Projekt Terrarium): simulierter Sensor, Erfassung und Umrechnung
gcc -std=gnu11 -Wall -Wextra -o "$OUT/dht_test" -I projects/terrarium/bsp/include tests/host/dht_test.c
"$OUT/dht_test"
