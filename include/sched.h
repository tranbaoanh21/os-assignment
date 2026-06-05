#ifndef SCHED_H
#define SCHED_H

#include "common.h"

#ifndef MLQ_SCHED
#define MLQ_SCHED
#endif

#define MAX_PRIO 140

int queue_empty(void);

void init_scheduler(void);
void finish_scheduler(void);

/* Get the next process from ready queue */
struct pcb_t * get_proc(void);

/* Put a process back to run queue */
void put_proc(struct pcb_t * proc);

/* Add a new process to ready queue */
void add_proc(struct pcb_t * proc);

/* Remove a finished process from the running list */
void finish_proc(struct pcb_t * proc);

/* Kernel-only PID lookup. The scheduler lock protects all queue traversal. */
struct pcb_t *find_proc_by_pid(uint32_t pid);

#endif
