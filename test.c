#include <stdio.h>
#include <pthread.h>
#include "queue.h"
#include "atomic_counter.h"

#define ASSERT(x, msg) do { \
    if (!x) { \
        printf("%s\n", msg); \
        exit(1); \
    } \
} while(0);

void int_queue_tests_without_resizing(void);
void int_queue_tests_with_resizing(void);
void atomic_counter_tests(void);
void *thread_dec_counter(void *);
void *thread_inc_counter(void *);
void *queue_pop_thread(void *);

int main()
{
    int_queue_tests_without_resizing();
    int_queue_tests_with_resizing();
    atomic_counter_tests();
}

void atomic_counter_tests()
{
    printf("\n\n TESTING ATOMIC COUNTER \n\n");
    
    struct atomic_counter counter;
    init_counter(&counter);
    pthread_t t1, t2, t3;
    
    pthread_create(&t1, NULL, (void *) thread_inc_counter, (void *) &counter);
    pthread_create(&t2, NULL, (void *) thread_inc_counter, (void *) &counter);
    pthread_create(&t3, NULL, (void *) thread_inc_counter, (void *) &counter);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    ASSERT (val_counter(&counter) == 3, "Counter value should be 3\n");

    pthread_create(&t1, NULL, (void *) thread_dec_counter, (void *) &counter);
    pthread_create(&t2, NULL, (void *) thread_dec_counter, (void *) &counter);
    pthread_create(&t3, NULL, (void *) thread_dec_counter, (void *) &counter);
    
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    ASSERT (val_counter(&counter) == 0, "Counter value should be 0\n");
}

void *thread_inc_counter(void *counter_)
{
    struct atomic_counter *counter = (struct atomic_counter *)counter_;
    inc_counter(counter);
    printf("Counter is now %zu\n", val_counter(counter));
}

void *thread_dec_counter(void *counter_)
{
    struct atomic_counter *counter = (struct atomic_counter *)counter_;
    dec_counter(counter);
    printf("Counter is now %zu\n", val_counter(counter));
}

void int_queue_tests_with_resizing()
{
    printf("\n\n TESTING QUEUE WITH RESIZING \n\n");
    struct int_queue queue;
    init_queue(&queue);

    pthread_t t1, t2, t3;

    printf("Now we are adding current_size + 3 elements into the queue, so the queue will have to resize.\n");
    int i;
    int n = queue.current_size;
    for (i = 0; i < n + 3; i++)
    {
        queue_push(&queue, i);
    }

    printf("We are now popping current_size elements off the queue. They should be in order from 0 to 9.\n");
    for (i = 0; i < n; i++)
    {
        printf("Popping: %i\n", queue_pop(&queue));
    }

    printf("Now let's let the threads pop off the last three elements\n");

    pthread_create(&t1, NULL, (void *) queue_pop_thread, (void *) &queue);
    pthread_create(&t2, NULL, (void *) queue_pop_thread, (void *) &queue);
    pthread_create(&t3, NULL, (void *) queue_pop_thread, (void *) &queue);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    printf("The queue should now be empty.\n");
    ASSERT(is_empty(&queue), "the queue isn't empty\n");

}

void int_queue_tests_without_resizing()
{
   printf("\n\n TESTING QUEUE WITHOUT RESIZING \n\n");
   
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
