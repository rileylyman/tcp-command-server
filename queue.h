#include <pthread.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>

#define DEFAULT_SIZE 10
#define GROWTH_RATE  4

/* A simple, thread-safe, blocking FIFO queue
 * of integer values (file descriptors). 
 * */
struct int_queue
{
    int *values;
    int front_index;
    int back_index;

    size_t current_size;
    pthread_mutex_t mutex;
    pthread_cond_t  ready;
};

void init_queue(struct int_queue _[static 1]);
void destroy_queue(struct int_queue _[static 1]);
void queue_push(struct int_queue _[static 1], int);
int queue_pop(struct int_queue _[static 1]);
void resize(struct int_queue _[static 1], size_t);
int is_empty(struct int_queue _[static 1]);

/* Initializes the given queue. This function must be
 * called before use of the queue.
 * */
void init_queue(struct int_queue queue[static 1])
{
    queue->values = (int *) calloc(DEFAULT_SIZE, sizeof(int));
    queue->front_index = 0;
    queue->back_index = 0;
    queue->current_size = DEFAULT_SIZE;

    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->ready, NULL);
}

/* Destroys the queue's mutex and condition variables, 
 * along with freeing queue->values.
 * */
void destroy_queue(struct int_queue queue[static 1])
{
    if (queue->values == NULL)
    {
        printf("This queue may already be destroyed, or was never initialized\n");
        return;
    }
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->ready);
    free(queue->values);
    queue->values = NULL;
}

/* Push an element onto the back of the queue, then signal
 * any waiting threads that there is a new element ready.
 * */
void queue_push(struct int_queue queue[static 1], int value)
{
    pthread_mutex_lock(&queue->mutex);

    queue->values[queue->front_index] = value;
    
    int new_front_index = (queue->front_index + 1) % queue->current_size;
    
    /* new_front_index is the next place front_index will point to. If
     * that is the same as back_index, then we know we have run out of space
     * in our values array. Therefore, we will now resize the array. Resizing
     * takes care of incrementing front_index, so we do not need to increment
     * as usual in this case. */
    if (new_front_index == queue->back_index)
        resize(queue, queue->current_size * GROWTH_RATE);
    else
        queue->front_index = new_front_index;
    
    pthread_cond_signal(&queue->ready);
    pthread_mutex_unlock(&queue->mutex);
    
}

/* Pop off and return the element at the front of the queue.
 * This function blocks until there is a value ready.
 * */
int queue_pop(struct int_queue queue[static 1])
{
    pthread_mutex_lock(&queue->mutex);
    while (is_empty(queue))
        pthread_cond_wait(&queue->ready, &queue->mutex);
    
    int ret = queue->values[queue->back_index];
    queue->back_index = (queue->back_index + 1) % queue->current_size;

    pthread_mutex_unlock(&queue->mutex);
    return ret;
}

/* Returns whether or not the current queue is empty. 
 * */
inline int is_empty(struct int_queue queue[static 1])
{
    return queue->front_index == queue->back_index;
}


/* Resizes queue->values to contain new_size integer elements. This
 * function assumes that the calling thread has acquired queue->mutex.
 * If this is not the case, the behaviour of this function is undefined.
 *
 * This function also assumes that the number of elements currently
 * contained within the queue is less than or equal to new_size.
 * If this is not the case, this function will not transfer all queue
 * elements.
 * */
void resize(struct int_queue queue[static 1], size_t new_size)
{
    /* This will cause problems if we try to use new_size as a modulus.
     * Also, it does not make sense to have new_size less than 1. */
    if (new_size < 1) return;

    int idx;
    int new_front_index = 0;

    int *new_values = (int *) calloc(new_size, sizeof(int));
    
    for (idx = queue->back_index; idx != queue->front_index;
            idx = (idx + 1) % new_size)
    {
        new_values[new_front_index] = queue->values[idx];
        new_front_index += 1;

        /* Make sure no segfaults occur. */
        if (new_front_index == new_size) break;
    }
    new_values[new_front_index++] = queue->values[queue->front_index];

    queue->values = new_values;
    queue->back_index = 0;
    queue->front_index = new_front_index % new_size;
    queue->current_size = new_size;
}

