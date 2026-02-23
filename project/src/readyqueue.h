#ifndef READYQUEUE_H
#define READYQUEUE_H

#include "pcb.h"

void rq_init();
int rq_is_empty();

void rq_enqueue(PCB *p);
PCB *rq_dequeue();
PCB *rq_peek();

#endif