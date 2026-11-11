#!/usr/bin/env bash
# Baut alle Programmvarianten (Release) und sammelt nur die .hex-Dateien in dist/.
#   normal                -> dist/stm32f746g_normal.hex
#   Fehlertest-Varianten  -> dist/stm32f746g_faulttest_<art>.hex   (siehe docs/architektur.md)
set -euo pipefail
cd "$(dirname "$0")/.."

FAULT_TESTS="nullptr div0 undef execsdram stackoverflow assert malloc"
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
}

build normal -DAPP_FAULT_TEST=
for t in $FAULT_TESTS; do
    build "faulttest_$t" -DAPP_FAULT_TEST="$t"
done
