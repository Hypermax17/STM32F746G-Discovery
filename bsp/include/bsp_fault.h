#ifndef BSP_FAULT_H
#define BSP_FAULT_H

/* Fehlerbehandlung: NMI, HardFault, MemManage, BusFault und UsageFault zeigen
 * Register (PC, LR, R0-R3, R12, xPSR, SP), Fault-Status (CFSR, HFSR, MMFAR, BFAR),
 * die betroffene Task und die dekodierte Ursache auf dem Display. Danach blinkt
 * LD1 7x. Die Handler ueberschreiben die schwachen Symbole aus dem Startup-Code;
 * die Aktivierung der Fault-Klassen geschieht in bsp_system_init_early(). */

#endif
