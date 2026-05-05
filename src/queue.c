#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t *q)
{
        if (q == NULL)
                return 1;
        return (q->size == 0);
}

void enqueue(struct queue_t *q, struct pcb_t *proc)
{
        /* TODO: put a new process to queue [q] */
        if (q == NULL || proc == NULL)
                return;

        if (q->size >= MAX_QUEUE_SIZE)
                return;

        q->proc[q->size] = proc;
        q->size++;
}

struct pcb_t *dequeue(struct queue_t *q)
{
        /* TODO: return a pcb whose prioprity is the highest
         * in the queue [q] and remember to remove it from q
         * */
        if (q == NULL || q->size == 0)
                return NULL;

        /* Use FIFO within a queue to preserve arrival order.
         * Priority-based selection is handled by MLQ buckets or
         * higher-level scheduler logic. */
        struct pcb_t *ret = q->proc[0];
        int i;
        for (i = 0; i < q->size - 1; i++)
                q->proc[i] = q->proc[i + 1];
        q->size--;

        return ret;
}

struct pcb_t *purgequeue(struct queue_t *q, struct pcb_t *proc)
{
        /* TODO: remove a specific item from queue
         * */
        if (q == NULL || proc == NULL || q->size == 0)
                return NULL;

        int i;
        for (i = 0; i < q->size; i++) {
                if (q->proc[i] == proc) {
                        struct pcb_t *ret = q->proc[i];
                        for (; i < q->size - 1; i++)
                                q->proc[i] = q->proc[i + 1];
                        q->size--;
                        return ret;
                }
        }

        return NULL;
}