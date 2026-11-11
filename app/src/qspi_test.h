#ifndef QSPI_TEST_H
#define QSPI_TEST_H

/* Schreibtest fuer den QSPI-Flash. Der Test beruehrt nur einen 4-KB-Sektor, der komplett leer
 * (0xFF) ist - vorhandene Daten bleiben unangetastet. Er sucht von hinten (Flash-Ende) nach
 * einem solchen Sektor, programmiert ein Muster, vergleicht es (indirekt und per Memory-Mapping),
 * und loescht den Sektor danach wieder. Ergebnis und Zeiten stehen im Log. */
void qspi_write_test(void);

#endif
