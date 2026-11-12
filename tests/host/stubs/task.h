#ifndef FAKE_TASK_H
#define FAKE_TASK_H
#define taskSCHEDULER_NOT_STARTED 1
#define taskSCHEDULER_RUNNING 2
static inline int xTaskGetSchedulerState(void) { return taskSCHEDULER_NOT_STARTED; }
#endif
