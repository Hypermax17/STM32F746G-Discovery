#ifndef FAULT_TEST_H
#define FAULT_TEST_H

/* Startet (nur bei -DAPP_FAULT_TEST=<art>) eine Task, die nach 3 s einen Fehler provoziert. */
void fault_test_start(void);

#endif
