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
    PCB *curr;

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
    curr = rq_head;
    while (curr->next != NULL && curr->next->scriptLength <= p->scriptLength) {
        curr = curr->next;
    }

    p->next = curr->next;
    curr->next = p;

    // if added to end update tail 
    if (p->next == NULL) 
        rq_tail = p;
}

// sorted enqueue by score (lowest score = highest priority)
// for aging scheduler
void rq_enqueue_score(PCB *p) {
    PCB *curr;
    
    if (!p)
    return;
    p->next = NULL;
    
    // empty queue
    if (rq_head == NULL) { 
        rq_head = rq_tail = p; 
        return; 
    }
    
    // new node has lowest score = highest priority, insert @ front
    if (p->score < rq_head->score) {
        p->next = rq_head;
        rq_head = p;
        return;
    }
    
    // iterate thru nodes until next node has higher score, then insert
    curr = rq_head;
    while (curr->next != NULL && curr->next->score <= p->score) {
        curr = curr->next;
    }
    
    p->next = curr->next;
    curr->next = p;
    
    // update tail if inserted @ back
    if (p->next == NULL) 
    rq_tail = p;
    
}

void rq_enqueue_front(PCB *p) {
    if (!p) 
        return;

    // check if queue empty
    if (rq_head == NULL) {
        p->next = NULL;
        rq_head = rq_tail = p;
        return;
    }

    // push onto front
    p->next = rq_head;
    rq_head = p;
}

// age all PCBs that are waiting (for running AGING scheduler)
void rq_age_all() {
    PCB *curr;
    for (curr = rq_head; curr != NULL; curr = curr->next) {
        if (curr->score > 0)
            curr->score--;
    }
}

// remove and return process @front of queue
PCB *rq_dequeue() {
    PCB *p;

    if (rq_head == NULL)
        return NULL;

    p = rq_head;
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

