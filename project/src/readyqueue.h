#ifndef READYQUEUE_H
#define READYQUEUE_H

#include "pcb.h"

// core
void rq_init();
int rq_is_empty();

// enqueues
void rq_enqueue(PCB *p);
void rq_enqueue_sjf(PCB *p);
void rq_enqueue_score(PCB *p);
void rq_enqueue_front(PCB *p);

// dequeues
PCB *rq_dequeue();
PCB *rq_dequeue_score();

// aging
void rq_age_all();

#endif