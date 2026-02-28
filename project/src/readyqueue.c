#include "readyqueue.h"
// RQ is a FIFO linked list

PCB *rq_head = NULL;
PCB *rq_tail = NULL;


// CORE 
// resets queue
void rq_init() {
    rq_head = NULL;
    rq_tail = NULL;
}

// return 1 if queue empty, else 0
int rq_is_empty() {
    return rq_head == NULL;
}

// ENQUEUES
// add process to back of queue (FIFO)
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

// insert a process in sorted order by script length (shortest first) for SJF
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

// insert a process in sorted order by score (lowest score = highest priority) for AGING 
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
    if (p->score <= rq_head->score) {
        p->next = rq_head;
        rq_head = p;
        return;
    }
    
    // iterate thru nodes until next node has higher score, then insert
    curr = rq_head;
    while (curr->next != NULL && curr->next->score < p->score) {
        curr = curr->next;
    }
    
    p->next = curr->next;
    curr->next = p;
    
    // update tail if inserted @ back
    if (p->next == NULL) 
        rq_tail = p;
    
}

// insert process @ front of RQ
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

// DEQUEUES
// remove and return process @ front of queue
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

// removes and returns the process w/ the lowest score (for AGING)
PCB* rq_dequeue_score() {
    if (!rq_head) 
    return NULL;
    
    // assumption = head process is the best candidate
    PCB *best = rq_head;
    PCB *best_prev = NULL;

    PCB *prev = rq_head;
    PCB *curr = rq_head->next;

    // find lowest score
    while (curr) {
        if (curr->score < best->score) {
            best = curr;
            best_prev = prev;
        }
        prev = curr;
        curr = curr->next;
    }

    // remove best from queue
    if (best_prev == NULL) { 
        rq_head = best->next;
    } else {
        best_prev->next = best->next;
    }
    
    // if removed tail, update ptr
    if (best == rq_tail) 
    rq_tail = best_prev;
    
    best->next = NULL; // detach before returning
    return best;
}

// AGING
// decrement the score of all waiting processes by 1 s.t. min=0 (for AGING)
void rq_age_all() {
    PCB *curr;
    for (curr = rq_head; curr != NULL; curr = curr->next) {
        if (curr->score > 0)
            curr->score--;
    }
}