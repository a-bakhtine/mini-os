#include "readyqueue.h"

PCB *rq_head = NULL;
PCB *rq_tail = NULL;

void rq_init() {
    rq_head = NULL;
    rq_tail = NULL;
}

int rq_is_empty() {
    return rq_head == NULL;
}

void rq_enqueue(PCB *p) {
    if (p == NULL)
        return;

    p -> next = NULL;

    if (rq_tail == NULL) { // empty queue
        rq_head = p;
        rq_tail = p;
    } else {
        rq_tail -> next = p;
        rq_tail = p;
    }
}

PCB *rq_dequeue() {
    if (rq_head == NULL)
        return NULL;

    PCB *p = rq_head;
    rq_head = rq_head->next;

    if (rq_head == NULL) // queue became empty
        rq_tail = NULL;

    p -> next = NULL; // detach
    return p;
}

PCB *rq_peek() {
    return rq_head;
}