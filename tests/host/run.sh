#!/usr/bin/env bash
# Rendert Konsole und UI-Seiten auf dem PC (ohne Board) und schreibt PNGs nach $OUT (Standard: build/host-render).
# Benutzt dieselben Quelldateien wie die Firmware (bsp_console, bsp_gfx, bsp_font, ui_render).
set -euo pipefail
cd "$(dirname "$0")/../.."
export OUT="${OUT:-build/host-render}"
mkdir -p "$OUT"
gcc -std=gnu11 -Wall -Wextra -o "$OUT/render_test" \
    -I tests/host/stubs -I bsp/include -I app/src \
    tests/host/render_test.c tests/host/fake_lcd.c \
    bsp/src/bsp_console.c bsp/src/bsp_gfx.c bsp/src/bsp_font.c app/src/ui_render.c
"$OUT/render_test"
python3 tests/host/topng.py
