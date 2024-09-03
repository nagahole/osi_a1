#ifndef QUEUE_H
#define QUEUE_H 1

struct queue
{
    int _start;
    int _size;
    int* _array;
    int _capacity;
};

struct queue* queue_create(int capacity);
int queue_enq(struct queue* queue, int val);
int queue_deq(struct queue* queue);
void queue_clear(struct queue* queue);
void queue_destroy(struct queue** queue);
int queue_len(struct queue* queue);

#endif
