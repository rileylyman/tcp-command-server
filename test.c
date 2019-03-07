#include <stdio.h>
#include <pthread.h>
#include "queue.h"

#define ASSERT(x, msg) do { \
    if (!x) { \
        printf("%s\n", msg); \
        exit(1); \
    } \
} while(0);

void int_queue_tests_without_resizing(void);
void *queue_pop_thread(void *);

int main()
{
    int_queue_tests_without_resizing();
}

void int_queue_tests_without_resizing()
{
   struct int_queue queue;
   init_queue(&queue);

   ASSERT(queue.values[0] == 0, "It seems that "
           "queue->values is unitialized.");

   pthread_t t1, t2, t3;
   pthread_create(&t1, NULL, (void *) queue_pop_thread, (void *) &queue);
   pthread_create(&t2, NULL, (void *) queue_pop_thread, (void *) &queue); 
   pthread_create(&t3, NULL, (void *) queue_pop_thread, (void *) &queue);
   
   printf("We should see the values 1, 2, then 3 printed now. Note that this could happen in any order because of thread scheduling.\n");

   queue_push(&queue, 1);
   queue_push(&queue, 2);
   queue_push(&queue, 3);

   pthread_join(t1, NULL);
   pthread_join(t2, NULL);
   pthread_join(t3, NULL);

   ASSERT(is_empty(&queue), "The queue should now be empty");
   printf("The queue is empty, as expected\n");

   queue_push(&queue, 1);
   queue_push(&queue, 2);
   queue_push(&queue, 3);
   
   printf("We should now see 1, 2, and 3 printed in order since we are no longer using threads to pop the values\n");

   printf("Popped %i off of queue\n", queue_pop(&queue));
   printf("Popped %i off of queue\n", queue_pop(&queue));
   printf("Popped %i off of queue\n", queue_pop(&queue));
   
   ASSERT(is_empty(&queue), "The queue should now be empty");
   printf("The queue is empty, as expected\n");
}

void *queue_pop_thread(void *queue_)
{
    struct int_queue *queue = (struct int_queue *)queue_;
    int ret = queue_pop(queue);
    printf("Popped %i off of queue\n", ret);
}
