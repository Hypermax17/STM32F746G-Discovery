# Flashen mit STM32CubeProgrammer

Das Board wird ueber den integrierten ST-LINK (USB-Buchse CN14 "USB ST-LINK") geflasht.

1. Board per USB mit dem PC verbinden (Buchse **CN14**, ST-LINK).
2. STM32CubeProgrammer starten, rechts oben **ST-LINK** waehlen, **Mode: Normal**, **Port: SWD** und **Connect**.
3. Reiter **Erasing & Programming** (Symbol "Download"):
   - **File path:** `stm32f746g_app.bin`, `.hex` oder `.elf` (aus `build/release/app/` oder dem CI-Artefakt)
   - **Start address:** `0x08000000` (nur bei `.bin` noetig; `.hex`/`.elf` enthalten die Adresse)
   - Haken bei **Verify programming** und **Run after programming**
4. **Start Programming**.

Nach dem Programmieren startet die Applikation: das Display zeigt das Boot-Banner, LED LD1 (gruen) blinkt.

## Wo bekomme ich die Binaries?

Die Binaries werden nicht eingecheckt, sondern gebaut:

- **GitHub Actions:** Repository -> Reiter *Actions* -> Lauf des gewuenschten Commits -> unten *Artifacts*.
  - `stm32f746g-release` / `stm32f746g-debug`: normale Applikation
  - `stm32f746g-faulttest-<art>`: Varianten, die 3 s nach dem Start einen Fehler ausloesen
    (`nullptr`, `div0`, `undef`, `execsdram`, `stackoverflow`, `assert`, `malloc`)
  - Jedes Artefakt ist ein ZIP mit `.bin`, `.hex` und `.elf`.
- **Lokal:** siehe README (`build/release/app/`).

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
