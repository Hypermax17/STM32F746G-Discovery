#!/usr/bin/env bash
# Baut Bootloader und Applikation (Release) und legt die .hex-Dateien in dist/ ab.
#   Bootloader -> dist/stm32f746g_bootloader.hex  (Flash ab 0x08000000, einmalig per ST-LINK)
#   normal     -> dist/stm32f746g_normal.hex      (App-Image ab 0x08040000)
# Optional zusaetzlich Fehlertest-Varianten (siehe docs/architektur.md), z. B.:
#   FAULT_TESTS="nullptr div0" scripts/build_variants.sh
#   -> dist/stm32f746g_faulttest_<art>.hex
set -euo pipefail
cd "$(dirname "$0")/.."

FAULT_TESTS="${FAULT_TESTS:-}"
OUT=dist
rm -rf "$OUT"
mkdir -p "$OUT"

build() {  # build <name> [cmake-args...]
    local name=$1; shift
    local dir=build/variants/$name
    cmake --preset release -B "$dir" "$@" > /dev/null
    cmake --build "$dir" > /dev/null
    cp "$dir/app/stm32f746g_app.hex" "$OUT/stm32f746g_${name}.hex"
    echo "ok: $OUT/stm32f746g_${name}.hex"
    if [ "$name" = normal ]; then
        cp "$dir/bootloader/stm32f746g_bootloader.hex" "$OUT/stm32f746g_bootloader.hex"
        echo "ok: $OUT/stm32f746g_bootloader.hex"
    fi
}

build normal -DAPP_FAULT_TEST=
for t in $FAULT_TESTS; do
    build "faulttest_$t" -DAPP_FAULT_TEST="$t"
done
