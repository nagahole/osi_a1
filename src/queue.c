#include <queue.h>

#include <stdint.h>
#include <stdlib.h>

struct queue* queue_create(int capacity)
{
    struct queue* queue = malloc(sizeof(struct queue));

    if (queue == NULL) // malloc failed
    {
        return NULL;
    }

    queue->_start = 0;
    queue->_size = 0;

    queue->_capacity = capacity;
    queue->_array = malloc(capacity * sizeof(int));

    return queue;
}

int queue_enq(struct queue* queue, int val)
{
    if (queue->_size >= queue->_capacity)
    {
        int* new_arr = malloc(queue->_capacity * 2 * sizeof(int));

        if (new_arr == NULL) // malloc failed
        {
            return -1;
        }

        for (int i = 0; i < queue->_capacity; i++)
        {
            new_arr[i] = queue->_array[(queue->_start + i) % queue->_capacity];
        }

        free(queue->_array);

        queue->_start = 0;
        queue->_size = queue->_capacity;
        queue->_capacity *= 2;
        queue->_array = new_arr;
    }

    queue->_array[(queue->_start + queue->_size) % queue->_capacity] = val;
    queue->_size++;

    return 0;
}

int queue_deq(struct queue* queue)
{
    if (queue->_size == 0)
    {
        return INT32_MIN;
    }

    int res = queue->_array[queue->_start];

    queue->_size--;
    queue->_start = (queue->_start + 1) % queue->_capacity;

    return res;
}

void queue_clear(struct queue* queue)
{
    queue->_start = 0;
    queue->_size = 0;
}

void queue_destroy(struct queue** queue)
{
    free((*queue)->_array);
    free(*queue);
    *queue = NULL;
}

int queue_len(struct queue* queue)
{
    return queue->_size;
}
