#include <pthread.h>
#include <sys/types.h>

/* A simple thread-safe counter.*/
struct atomic_counter
{
    size_t count;
    pthread_mutex_t lock;
};

/* Initialize the counter. */
void counter_init(struct atomic_counter *counter)
{
    counter->count = 0;
    pthread_mutex_init(&counter->lock, NULL);
}

/* Increment the counter. */
void inc_counter(struct atomic_counter *counter)
{
    pthread_mutex_lock(&counter->lock);
    counter->count++;
    pthread_mutex_unlock(&counter->lock);
}

/* Decrement the counter. */
void dec_counter(struct atomic_counter *counter)
{
    pthread_mutex_lock(&counter->lock);
    counter->count--;
    pthread_mutex_unlock(&counter->lock);
}

/* Get the counter's current value. */
int val_counter(struct atomic_counter *counter)
{
    size_t ret;
    pthread_mutex_lock(&counter->lock);
    ret = counter->count;
    pthread_mutex_unlock(&counter->lock);
    return ret;
}
