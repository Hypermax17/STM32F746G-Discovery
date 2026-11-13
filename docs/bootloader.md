# Bootloader und Netzwerk-Update

Ziel: Das Board haengt im lokalen Netz und nimmt neue Applikationen per Ethernet entgegen, ohne dass ein PC per ST-LINK
angeschlossen sein muss. Der Bootloader ist dafuer die feste Basis; er wird nur ueber ST-LINK geflasht.

## Flash-Aufteilung

| Adresse | Groesse | Inhalt |
|---|---|---|
| `0x08000000` | 256 KB (Sektor 0..4) | Bootloader (`bootloader/`, ca. 24 KB belegt) |
| `0x08040000` | 768 KB (Sektor 5..7) | Applikation (derzeit ca. 420 KB) |

Der STM32F746 hat nur **eine** Flash-Bank, ein A/B-Wechsel ist nicht moeglich. Sicherheit kommt aus dem Ablauf: das neue Image wird
komplett empfangen und per CRC geprueft, bevor die App geloescht wird; der Bootloader selbst wird nie ueberschrieben. Faellt der Strom
waehrend des Schreibens aus, bleibt die App ungueltig und der Bootloader wartet beim naechsten Start auf ein neues Image.

Die Konstanten stehen in `common/include/image.h` und in `linker/bootloader.ld` / `linker/app.ld`.

## Image-Format

Die App wird bei `0x08040000` gelinkt. Direkt hinter der Vektortabelle (Offset `0x200`) liegt ein 64-Byte-Header
(`image_header_t`: Magic, Groesse, Linkadresse, Versionstext). Nach dem Linken traegt `scripts/make_image.py` die Groesse ein und haengt
eine CRC32-Pruefsumme (wie `zlib.crc32`) an. `stm32f746g_app.bin` ist genau der Flash-Inhalt ab `0x08040000`; die `.hex` wird daraus erzeugt.

Der Bootloader startet die App nur, wenn Header, Groesse, Linkadresse, Vektortabelle und CRC stimmen (`common/src/image.c`).

## Ablauf beim Start

1. Display und Konsole initialisieren, Version anzeigen.
2. App-Image pruefen und Ergebnis anzeigen.
3. Gueltig und Taster **B1** nicht gedrueckt: nach 1,5 s in die App springen (Takt auf HSI, Peripherie zurueckgesetzt, Caches/MPU aus,
   `VTOR` auf die App-Vektortabelle).
4. Sonst bleibt der Bootloader aktiv (LED blinkt). Ab Schritt 3 wartet er dann auf ein Update per Netzwerk.

## Umsetzungsplan

| Schritt | Inhalt | Stand |
|---|---|---|
| 0 | Speicherlayout, Image-Format, Bootloader startet die App | umgesetzt |
| 1 | Ethernet-Treiber im BSP (RMII, LAN8742A, MPU/Cache fuer DMA-Puffer), Link-Anzeige | offen |
| 2 | lwIP (ohne RTOS), DHCP, Ping, IP auf dem Display | offen |
| 3 | HTTP-Server: Upload ins SDRAM, CRC-Pruefung | offen |
| 4 | Flash-Schreiben (Sektor 5..7), Verifizieren, App starten; Upload-Skript | offen |
| 5 | Start des Bootloaders aus der App per Netzwerk, CI/Doku | offen |
