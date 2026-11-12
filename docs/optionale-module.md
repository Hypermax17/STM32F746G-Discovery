# Optionale Module (Geruest): microSD, USB OTG, Ethernet, Kamera

Diese vier Module werden **noch nicht benutzt**. Das BSP enthaelt dafuer nur ein Geruest, damit sie spaeter
ohne Umbau der Applikation eingebaut werden koennen:

| Teil | Ort |
|---|---|
| API (Funktionen, Typen) | `bsp/include/bsp_sd.h`, `bsp_usb.h`, `bsp_eth.h`, `bsp_camera.h` (gemeinsame Fehlercodes: `bsp_common.h`) |
| Platzhalter (Stub) | `bsp/src/optional/bsp_<modul>_stub.c` - alle Funktionen liefern `BSP_ERR_NOT_IMPLEMENTED` (Abfragen wie `bsp_sd_card_present()` liefern 0) |
| Echter Treiber (spaeter) | `bsp/src/optional/bsp_<modul>.c` |
| Schalter | CMake-Option `BSP_ENABLE_SD / _USB / _ETH / _CAMERA` (Standard `OFF`) |
| Zustand zur Compile-Zeit | `BSP_HAS_SD / _USB / _ETH / _CAMERA` = 0 oder 1 (fuer `#if` in der App) |

Mit `-DBSP_ENABLE_SD=ON` bricht CMake mit einer Meldung ab, solange `bsp_sd.c` nicht existiert.
Die Applikation kann die API schon heute aufrufen und auf `BSP_ERR_NOT_IMPLEMENTED` reagieren.

## Vorgehen beim Einbau eines Moduls

1. Treiber `bsp/src/optional/bsp_<modul>.c` schreiben (API aus dem Header umsetzen, Thread-Sicherheit wie in den
   anderen Treibern: Mutex, Interrupt-Prioritaet >= 5 bei FreeRTOS-Aufrufen).
2. Benoetigte HAL-Module in `bsp/config/stm32f7xx_hal_conf.h` einschalten (`HAL_<X>_MODULE_ENABLED`, Include,
   `USE_HAL_<X>_REGISTER_CALLBACKS 0U`) und die Quelldateien in `third_party/CMakeLists.txt` (`HAL_MODULES`) ergaenzen.
3. Falls das Modul MPU/Cache betrifft (DMA-Puffer, Speicherbereiche): Regionen in `bsp_system.c` pruefen.
4. Mit `-DBSP_ENABLE_<MODUL>=ON` bauen und im Boot-Log/auf der Systemseite pruefen.
5. Middleware optional als Submodul unter `third_party/` einbinden (siehe Tabelle).

## Modulspezifisch

Die Pins stammen aus dem Gedaechtnis des ST-Referenz-BSP und sind **nicht gegen das Board-Handbuch
(UM1907) und den Schaltplan geprueft**. Vor der Inbetriebnahme dort kontrollieren.

| Modul | Peripherie | HAL-Module | Pins (ungeprueft) | Middleware (Vorschlag) |
|---|---|---|---|---|
| microSD | SDMMC1, 4-Bit | `hal_sd`, `ll_sdmmc` (+ DMA2) | D0..D3 = PC8..PC11, CK = PC12, CMD = PD2, Karte-erkannt = PC13 | FatFs |
| USB OTG | OTG FS (CN12) / OTG HS mit ULPI-PHY (CN13) | `hal_pcd`, `hal_hcd`, `ll_usb` | FS: DM = PA11, DP = PA12, ID = PA10; HS (ULPI): siehe Schaltplan | ST USB Device/Host Library oder TinyUSB |
| Ethernet | ETH (RMII), PHY LAN8742A | `hal_eth` | REF_CLK = PA1, MDIO = PA2, CRS_DV = PA7, MDC = PC1, RXD0/1 = PC4/PC5, TX_EN = PG11, TXD0/1 = PG13/PG14 | lwIP |
| Kamera | DCMI (8-Bit), Sensor-Steuerung ueber I2C | `hal_dcmi` (+ DMA2) | siehe Schaltplan (Kamerastecker); einige Leitungen koennen mit SDRAM/LCD kollidieren - vorher pruefen | - |

Nutzbare Ressourcen: SDRAM ab `0xC0080000` (siehe `docs/architektur.md`) eignet sich fuer Kamera-/Netzwerkpuffer;
I2C3 ist belegt (Touch, Audio-Codec).
