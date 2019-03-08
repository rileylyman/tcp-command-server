#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "queue.h"
#include "atomic_counter.h"
#include <string.h>
#include <unistd.h>

#define THREAD_POOL_SIZE 4
#define READ_BUFFER_SIZE 1024

static struct int_queue clients_waiting;  /* A queue of all clients waiting to be served. */
static struct atomic_counter num_clients; /* The number of clients connected to the server. */
static int server_id; /* This server's ID. */

static int port; /* This server's port. */

typedef void command_handler(int); /* A function that handles one of the ASCII commands. */

/* Function prototypes: */
void initialize_thread_pool(struct int_queue *, size_t);
void serve(void);
void *thread_pool_worker(void *);
void handle_connection(int);
command_handler *parse_command(char *, size_t);
void handle_who(int);
void handle_where(int);
void handle_why(int);
void handle_command_not_found(int);
/* End function prototypes. */

int main(int argc, char **argv)
{
    server_id = 10;
    
    init_queue(&clients_waiting);
    init_counter(&num_clients);

    initialize_thread_pool(&clients_waiting, THREAD_POOL_SIZE);

    serve();
}

/* Sets up the server socket and begins to accept connections.
 * Every time a new connection is opened, it adds to to the 
 * queue of waiting connections to be served by the thread pool.
 * */
void serve()
{
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr, client_addr;
    size_t client_addr_len = sizeof(client_addr);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    listen(sockfd, 1024);

    while (1)
    {
        int client_fd = accept(sockfd, (struct sockaddr *) &client_addr, (socklen_t *) sizeof(client_addr));
        if (client_fd < 0)
        {
            printf("Error accepting connection.\n");
            continue;
        }
        printf("Accepted connection from %s on port %d\n", 
                inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
        queue_push(&clients_waiting, client_fd);
    }
    
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
}

/* Below are the function handlers for each command. 
 * */
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
    printf("Got command: %s\n", buffer);

    if (! strcmp(buffer, "WHO"))
        return handle_who;
    else if (! strcmp(buffer, "WHERE"))
        return handle_where;
    else if (! strcmp(buffer, "WHY"))
        return handle_why;
    else 
        return handle_command_not_found;
}

/* Each thread calls this function when it pops a new connection
 * of the waiting connections queue. It reads the command from the
 * connection and then sends the appropriate response.
 * */
void handle_connection(int conn_fd)
{
    size_t amt;
    char buffer[READ_BUFFER_SIZE] = {'\0'};
    while ((amt = read(conn_fd, buffer, READ_BUFFER_SIZE) > 0))
    {
        printf("Received %zu bytes\n", amt);
        buffer[amt] = '\0';
        command_handler *handler = parse_command(buffer, amt);
        handler(conn_fd);
        printf("\nHandled connection\n");
    }
    printf("Closing connection\n");
    close(conn_fd);
}

/* Creates `size` threads and makes them all try to pop an element
 * from the waiting connections queue. This will cause them all to 
 * block until a new connection arrives.
 * */
void initialize_thread_pool(struct int_queue *queue, size_t size)
{
    size_t i;
    for (i = 0; i < size; i++)
    {
        pthread_t t;
        pthread_create(&t, NULL, (void *) thread_pool_worker, (void *) queue);
    }
}

/* Attempts to pop a connection off the waiting connections queue. 
 * Then, increment num_clients, handle the connectio, and then 
 * decrement num_clients.
 * */
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
