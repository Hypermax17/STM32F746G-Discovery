#!/usr/bin/env bash
# Rendert die LVGL-Oberflaeche (alle Seiten in allen Designs) auf dem PC und schreibt PNGs nach $OUT
# (Standard: build/host-ui). Benoetigt nur gcc und python3, kein Board.
set -euo pipefail
cd "$(dirname "$0")/../.."
export OUT="${OUT:-build/host-ui}"
mkdir -p "$OUT/obj"

LVGL=third_party/lvgl
CFLAGS="-std=gnu11 -O1 -w -DUI_HOST_BUILD -DLV_CONF_INCLUDE_SIMPLE -Iui/config -I$LVGL"
SRC=$(cd $LVGL && find src -name '*.c' \
  | grep -v -E '^src/drivers/' \
  | grep -v -E '^src/draw/(nxp|renesas|sdl|opengles|vg_lite|nema_gfx|dma2d|espressif|eve|sw/arm2d|sw/blend/(arm2d|helium|neon))/' | sort)

# LVGL einmal als Objektdateien (parallel); wird nur neu gebaut, wenn lv_conf.h oder die Quelle neuer ist
for f in $SRC; do
    o="$OUT/obj/$(echo "$f" | tr '/' '_').o"
    if [ ! -f "$o" ] || [ "$LVGL/$f" -nt "$o" ] || [ ui/config/lv_conf.h -nt "$o" ]; then
        echo "gcc $CFLAGS -c $LVGL/$f -o $o"
    fi
done | xargs -P 8 -I{} sh -c '{}'

gcc -std=gnu11 -Wall -Wextra -O1 -DUI_HOST_BUILD -DLV_CONF_INCLUDE_SIMPLE \
    -Iui/config -I$LVGL -Iui/include -Iapp/src -Iapp/src/pages -Ibsp/include \
    -o "$OUT/ui_test" \
    tests/host/ui_test.c ui/src/ui_design.c ui/src/ui_widgets.c ui/src/ui_pages.c \
    app/src/app_ui.c app/src/pages/page_system.c app/src/pages/page_values.c app/src/pages/page_log.c \
    bsp/src/bsp_util.c "$OUT"/obj/*.o -lm
"$OUT/ui_test"
python3 tests/host/topng.py
