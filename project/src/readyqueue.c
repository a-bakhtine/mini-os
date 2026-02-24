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

// sorted enqueue for SJF
void rq_enqueue_sjf(PCB *p) {
    if (p == NULL) 
        return;
    p->next = NULL;

    // check empty queue
    if (rq_head == NULL) {
        rq_head = rq_tail = p;
        return;
    }

    // insert at head if shorter than curr head
    if (p->scriptLength < rq_head->scriptLength) {
        p->next = rq_head;
        rq_head = p;
        return;
    }

    // else search for where to insert
    PCB *cur = rq_head;
    while (cur->next != NULL && cur->next->scriptLength <= p->scriptLength) {
        cur = cur->next;
    }

    p->next = cur->next;
    cur->next = p;

    // if added to end update tail 
    if (p->next == NULL) 
        rq_tail = p;
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

