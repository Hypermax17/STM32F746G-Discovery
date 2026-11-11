# Flashen mit STM32CubeProgrammer

Das Board wird ueber den integrierten ST-LINK (USB-Buchse CN14 "USB ST-LINK") geflasht.

1. Board per USB mit dem PC verbinden (Buchse **CN14**, ST-LINK).
2. STM32CubeProgrammer starten, rechts oben **ST-LINK** waehlen, **Mode: Normal**, **Port: SWD** und **Connect**.
3. Reiter **Erasing & Programming** (Symbol "Download"):
   - **File path:** eine `.hex`-Datei aus dem CI-Artefakt `stm32f746g-hex` (lokal: `dist/` bzw. `build/release/app/`)
   - **Start address:** nicht noetig bei `.hex` (bei einer `.bin` waere es `0x08000000`)
   - Haken bei **Verify programming** und **Run after programming**
4. **Start Programming**.

Nach dem Programmieren startet die Applikation: das Display zeigt das Boot-Banner, LED LD1 (gruen) blinkt.

## Wo bekomme ich die Binaries?

GitHub: Repository -> Reiter *Actions* -> Lauf des gewuenschten Commits -> unten *Artifacts* ->
**`stm32f746g-hex`** (ein ZIP mit `stm32f746g_normal.hex`).

`.hex` enthaelt die Startadresse, im CubeProgrammer muss daher keine Adresse eingetragen werden.
Lokal erzeugt `scripts/build_variants.sh` dieselbe Datei im Ordner `dist/`.
Fehlertest-Varianten (siehe `docs/architektur.md`) baust du bei Bedarf lokal:
`FAULT_TESTS="nullptr div0" scripts/build_variants.sh`.

## Fehlersuche ohne Display: LED-Blinkcodes

Falls das Display nicht startet, blinkt LD1 in Gruppen (Pause dazwischen):

| Blinken | Bedeutung |
|---|---|
| 2x | Takt-Konfiguration (HSE/PLL) fehlgeschlagen |
| 3x | SDRAM-Initialisierung fehlgeschlagen |
| 4x | SDRAM-Test fehlgeschlagen |
| 5x | LCD-/Konsolen-Init fehlgeschlagen |
| 6x | Panic (Meldung steht auf dem Display, falls es laeuft) |
| 7x | CPU-Fault (Details stehen auf dem Display, falls es laeuft) |
