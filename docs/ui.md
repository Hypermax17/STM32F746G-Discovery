# Oberflaeche (UI-Framework auf Basis von LVGL)

Die Oberflaeche nutzt **LVGL v9.4** (MIT-Lizenz, Submodul `third_party/lvgl`). Der allgemeine Teil liegt in `ui/`,
die Seiten der jeweiligen Anwendung in `app/src/pages/`.

## Aufbau

| Teil | Ort | Aufgabe |
|---|---|---|
| LVGL-Konfiguration | `ui/config/lv_conf.h` | aus der LVGL-Vorlage erzeugt; nur benoetigte Widgets, Schriften Montserrat 12/14/16/20/28/48, Speicherpool im SDRAM |
| Anbindung ans Board | `ui/src/ui_port.c` | Display (zwei Vollbild-Puffer im SDRAM, Umschalten im vertikalen Austastintervall), Touch (Zeiger), Zeitbasis, Log, UI-Task |
| Designs | `ui/src/ui_design.c` | Farben, Schriften, Rundungen und Navigationsstil als Datenstruktur `ui_design_t` |
| Widget-Helfer | `ui/src/ui_widgets.c` | Seitengeruest mit Navigation, Karte, Label, Button, Status-Punkt, Tabelle, Diagramm |
| Seitenverwaltung | `ui/src/ui_pages.c` | mehrere Seiten, Wechsel mit Ueberblendung, periodisches Aktualisieren, Design wechseln |
| Seiten | `app/src/pages/` | `page_system.c`, `page_values.c`, `page_log.c` |
| Datenquellen | `app/src/data_hub.c`, `app/src/sysinfo.c`, BSP-Konsole | Messwert-Archiv, Systemzustand, Log-Ringpuffer |

Regeln:
- **LVGL ist nicht thread-sicher.** Alle LVGL-Aufrufe laufen im UI-Task. Andere Tasks melden nur Zustaende
  (`ui_port_touch_update`, `ui_port_request_next_page`) oder legen Daten im `data_hub`/Log-Ringpuffer ab; die Seiten holen sie
  per Update-Timer (500 ms).
- **Zustand gehoert nicht in die Widgets**, sondern in Datenquellen (`data_hub`, `sysinfo`, Log-Ringpuffer). Eine Seite kann dadurch jederzeit neu aufgebaut werden.
- Die Anwendung bekommt Messwerte **fertig in Einheiten** (Grad Celsius, Volt) vom BSP und rechnet nichts um.

## Basisdesign "Glas violett"

Alle Seiten nutzen dasselbe Design (`ui_design` in `ui/src/ui_design.c`): Verlauf von tiefem Violett nach Nachtblau, durchscheinende
Karten mit hellem Rand und weichem Schatten, grosse Rundungen, Akzent Violett (Hauptkurve, aktive Navigation), Rosa als zweite
Kurve, Mintgruen fuer "in Ordnung". Navigation ist eine **Seitenleiste links** (Symbol + Name, 76 px); der Inhaltsbereich ist
404 x 272 px gross. Schriften: Montserrat 12 / 14 / 20 / 28.

Farben, Deckkraft, Schatten, Rundung und Schriften stehen an einer Stelle (`ui_design_t`); die Seiten und Widgets
(`ui_widgets.c`) bauen sich daraus auf. Ein abweichendes Design fuer ein Projekt ist moeglich, indem eine zweite
`ui_design_t`-Konstante angelegt und an `ui_pages_init()` uebergeben wird.

Umlaute und Sonderzeichen (Grad-Zeichen) fehlen in den eingebauten Schriften; die Texte sind daher ASCII ("Loeschen", "41.3 C").

## Seiten

- **System (Cockpit-Anzeige):** drei runde Anzeigen (CPU-Last, CPU-Temperatur in Grad Celsius, VDDA in Volt), Status der Peripherie (Punkt gruen/rot), Heap und Laufzeit, **Live-Tabelle** der vier aktivsten Tasks (Prioritaet, freier Stack, CPU-Anteil), Test-Ton-Button.
- **Messwerte:** **Live-Diagramm** (2 Kurven mit eigener Y-Achse links/rechts, automatische Skalierung, 128 Punkte = ca. 2 min) und **Live-Tabelle** der letzten Messwerte, aktuelle Werte gross in der Kopfzeile, Buttons Pause/Loeschen.
- **Log:** Meldungen mit Farbe nach Schwere, scrollbar per Wischen, Buttons Pause/Loeschen.

Der Taster B1 (kurzer Druck) blaettert zur naechsten Seite.

## Neue Seite anlegen

1. `app/src/pages/page_xyz.c`: `lv_obj_t *page_xyz_create(const ui_design_t *d, unsigned idx)` aufbauen
   (`ui_frame_create(d, idx)` liefert Bildschirm, Navigation und Inhaltsbereich `f.content`, `f.w`, `f.h`), optional `page_xyz_update()`.
2. In `app/src/pages/pages.h` deklarieren, in `app/CMakeLists.txt` eintragen, in `app/src/app_ui.c` (`k_pages[]`, mit Name und Symbol) aufnehmen.

## Speicher

| Bereich | Adresse | Nutzung |
|---|---|---|
| SDRAM FB0 / FB1 | `0xC0000000` / `0xC0040000` | zwei Framebuffer (RGB565, je 255 KB); LVGL zeichnet im Hintergrundpuffer und schaltet im Austastintervall um |
| SDRAM UI | `0xC0080000`, 1 MB | LVGL-Speicherpool (`LV_MEM_ADR`) |
| SDRAM frei | ab `0xC0180000` | `.sdram`-Abschnitt fuer weitere Anwendungen |

## Darstellung ohne Board pruefen

```bash
tests/host/run_ui.sh        # alle Seiten -> PNGs in build/host-ui/
tests/host/run.sh           # Display-Konsole (Boot-Log, Fehleranzeige) und Rechentests
```

`run_ui.sh` uebersetzt LVGL, Designs, Widgets, Seitenverwaltung und die Seiten fuer den PC und rendert mit
synthetischen Daten. Benoetigt nur `gcc` und `python3`.

## Fehleranzeige

Bei Fehlern (`bsp_panic`, Fault-Handler) uebernimmt die BSP-Konsole den **angezeigten** Framebuffer und zeigt die Meldung ueber den
ganzen Bildschirm, unabhaengig von LVGL.
