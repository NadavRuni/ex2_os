#ifndef QUEUE_H
#define QUEUE_H

void ready_queue_push(int tid);
int ready_queue_pop();
int ready_queue_is_empty();
int ready_queue_size();

#endif
