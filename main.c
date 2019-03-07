#include "queue.h"

#define THREAD_POOL_SIZE 4

void initialize_thread_pool(struct int_queue *, size_t);

int main(int argc, char **argv)
{
    struct int_queue queue;
    init_queue(&queue);
    initialize_thread_pool(&queue, THREAD_POOL_SIZE);
}

void initialize_thread_pool(struct int_queue *queue, size_t size)
{
}
