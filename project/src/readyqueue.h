#ifndef READYQUEUE_H
#define READYQUEUE_H

#include "pcb.h"

void rq_init();
int rq_is_empty();

void rq_enqueue(PCB *p);
void rq_enqueue_sjf(PCB *p);
void rq_enqueue_score(PCB *p);
void rq_age_all();

PCB *rq_dequeue();
PCB *rq_peek();


#endif