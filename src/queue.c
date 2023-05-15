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
        if (q->size == MAX_QUEUE_SIZE)
                return;

        if (q->size == 0)
        {
                q->proc[0] = proc;
        }
        // nếu queue có phần tử và new proc priority nhỏ nhất
        // else if (proc->priority < q->proc[q->size - 1]->priority)
        // {
        //         q->proc[q->size] = proc;
        // }
        // nếu queue có phần tử và new proc priority nằm giữa
        else
        {
                // int i = 0;
                // while (proc->priority < q->proc[i]->priority)
                // {
                //         i++;
                // }
                // for (int j = q->size; j > i; j--)
                // {
                //         q->proc[j] = q->proc[j - 1];
                // }
                // q->proc[i] = proc;
                int i;
                for (i = q->size; i > 0 && proc->priority >= q->proc[i - 1]->priority; i--)
                {
                        // In one queue, if 2 processes have the same priority number,
                        // prioritize the ready one than the recently run one
                        q->proc[i] = q->proc[i - 1];
                } 

                q->proc[i] = proc;
        }
        q->size++;
}

struct pcb_t *dequeue(struct queue_t *q)
{
        /* TODO: return a pcb whose prioprity is the highest
         * in the queue [q] and remember to remove it from q
         * */
        if (empty(q))
                return NULL;

        struct pcb_t *proc = q->proc[q->size - 1];

        q->proc[q->size - 1] = NULL;
        q->size--;
        q->remaining_slots--;
        return proc;
}

// void enqueue(struct queue_t * q, struct pcb_t * proc) {
//         /* TODO: put a new process to queue [q] */
//         if (q->size == MAX_QUEUE_SIZE) return;

//         q->proc[q->size] = proc;
//         q->size++;
// }

// struct pcb_t * dequeue(struct queue_t * q) {
//         /* TODO: return a pcb whose prioprity is the highest
//          * in the queue [q] and remember to remove it from q
//          * */
//         if (empty(q))
//                 return NULL;

//         struct pcb_t *proc = q->proc[0];

//         for (int i=1; i<q->size; i++)
//         {
//                 q->proc[i-1] = q->proc[i];
//         }

//         //remove(q->proc[q->size-1]);
//         q->proc[q->size-1] = NULL;
//         q->size--;
//         q->remaining_slots--;
// 	return proc;
// }
