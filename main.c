#include <pthread.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "queue.h"
#include "atomic_counter.h"
#include <string.h>
#include <unistd.h>

#define THREAD_POOL_SIZE 4
#define READ_BUFFER_SIZE 1024

static struct int_queue clients_waiting;
static struct atomic_counter num_clients;
static int server_id;

typedef void command_handler(int);

void initialize_thread_pool(struct int_queue *, size_t);
void *thread_pool_worker(void *);
void handle_connection(int);
command_handler *parse_command(char *, size_t);

void handle_who(int);
void handle_where(int);
void handle_why(int);
void handle_command_not_found(int);

int main(int argc, char **argv)
{
    server_id = 10;
    
    init_queue(&clients_waiting);
    init_counter(&num_clients);

    initialize_thread_pool(&clients_waiting, THREAD_POOL_SIZE);

    int i;
    for (i = 0; i < 10; i++)
    {
        queue_push(&clients_waiting, 1);
    }
}

void handle_who(int conn_fd)
{
    char msg[20]; /*size_t should not be more than 20 digits in decimal.*/
    sprintf(msg, "%zu", val_counter(&num_clients));
    write(conn_fd, msg, strlen(msg));
}

void handle_where(int conn_fd)
{
    char msg[12]; /*int should not be more than 12 digits in decimal.*/
    sprintf(msg, "%d", server_id);
    write(conn_fd, msg, strlen(msg));
}

void handle_why(int conn_fd)
{
    write(conn_fd, "42", strlen("42"));
}

void handle_command_not_found(int conn_fd)
{
    char *msg = "Command not recognized";
    write(conn_fd, msg, strlen(msg));
}

/* Determines which command the buffer contains and returns a
 * function pointer that properly handles the command. This function
 * assumes that the buffer is null-terminated at `len`. 
 * */
command_handler *parse_command(char *buffer, size_t len)
{
    if (! strcmp(buffer, "WHO"))
        return handle_who;
    else if (! strcmp(buffer, "WHERE"))
        return handle_where;
    else if (! strcmp(buffer, "WHY"))
        return handle_why;
    else 
        return handle_command_not_found;
}

void handle_connection(int conn_fd)
{
    size_t amt;
    char buffer[READ_BUFFER_SIZE] = {'\0'};
    while ((amt = read(conn_fd, buffer, READ_BUFFER_SIZE) > 0))
    {
        buffer[amt] = '\0';
        command_handler *handler = parse_command(buffer, amt);
        handler(conn_fd);
        printf("Handled connection\n");
    }
}

void initialize_thread_pool(struct int_queue *queue, size_t size)
{
    size_t i;
    for (i = 0; i < size; i++)
    {
        pthread_t t;
        pthread_create(&t, NULL, (void *) thread_pool_worker, (void *) queue);
    }
}

void *thread_pool_worker(void *queue_)
{
    struct int_queue *queue = (struct int_queue *) queue_;

    for (;;) 
    {
        int connection_fd = queue_pop(queue);
        inc_counter(&num_clients);
        handle_connection(connection_fd);
        dec_counter(&num_clients);
    }
}
