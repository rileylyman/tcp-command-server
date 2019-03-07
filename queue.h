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
    
    queue->front_index = (queue->front_index + 1) % queue->current_size;
    if (queue->front_index == queue->back_index)
        resize(queue, queue->current_size * GROWTH_RATE);
    
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

inline int is_empty(struct int_queue queue[static 1])
{
    return queue->front_index == queue->back_index;
}

void resize(struct int_queue queue[static 1], size_t new_size)
{
}

