#include "readyqueue.h"
// RQ is a FIFO linked list

PCB *rq_head = NULL;
PCB *rq_tail = NULL;

// resets queue
void rq_init() {
    rq_head = NULL;
    rq_tail = NULL;
}

int rq_is_empty() {
    return rq_head == NULL;
}

// add process to back of queue
void rq_enqueue(PCB *p) {
    if (p == NULL)
        return;

    p->next = NULL;

    // check empty queue
    if (rq_tail == NULL) { 
        rq_head = p;
        rq_tail = p;
    } 
    // append 
    else {
        rq_tail->next = p;
        rq_tail = p;
    }
}

// remove and return process @front of queue
PCB *rq_dequeue() {
    if (rq_head == NULL)
        return NULL;

    PCB *p = rq_head;
    rq_head = rq_head->next;

    // check if queue empty
    if (rq_head == NULL) 
        rq_tail = NULL;

    p->next = NULL; 
    return p;
}

PCB *rq_peek() {
    return rq_head;
}