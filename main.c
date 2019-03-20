#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
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
#define BUFFER_LEN 2048

static struct int_queue clients_waiting;  /* A queue of all clients waiting to be served. */
static struct atomic_counter num_clients; /* The number of clients connected to the server. */
static int server_id; /* This server's ID. */

static int server_port = 8000; /* This server's port. */
const char *localhost = "127.0.0.1";

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
size_t read_command(int, char *, size_t);
int is_eol(char);
void client(void);
void send_commands(int);
/* End function prototypes. */

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: ./server [server, client]\n");
        exit(1);
    }
    if (!strcmp(argv[1], "client"))
    {
        client();
    }
    else if (!strcmp(argv[1], "server"))
    {
        server_id = 10;
        
        init_queue(&clients_waiting);
        init_counter(&num_clients);

        initialize_thread_pool(&clients_waiting, THREAD_POOL_SIZE);

        serve();
    }
}

/* Looks up the target IP address (in this case just localhost).
 * It then connects and allows the user to send ASCII commands
 * in a while loop.
 * */
void client()
{
    struct sockaddr_in target_address;
    memset(&target_address, 0, sizeof(target_address));
    target_address.sin_family = AF_INET;
    target_address.sin_port = htons(server_port);
    inet_aton("127.0.0.1", &target_address.sin_addr);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);

    int connection_status = connect(sockfd, 
            (struct sockaddr*) &target_address, sizeof(target_address));

    if (connection_status < 0)
    {
        printf("Could not connect\n");
        exit(1);
    }

    send_commands(sockfd);
}

void send_commands(int connection_fd)
{
    size_t amt_sent;
    char buffer[BUFFER_LEN] = {'\0'};
    for (;;) 
    {
        printf("Enter Command: ");
        fgets(buffer, BUFFER_LEN, stdin);
        
        if ((amt_sent = write(connection_fd, buffer, BUFFER_LEN)) == -1)
            perror("Could not write to server.");
        else
            printf("Sent %ld bytes\n", amt_sent);
        printf("Buffer: %s\n", buffer);
        read(connection_fd, buffer, BUFFER_LEN);
        printf("\nReceived %s\n", buffer);
    } 
    printf("Closing connection\n");
}

/* Sets up the server socket and begins to accept connections.
 * Every time a new connection is opened, it adds to to the 
 * queue of waiting connections to be served by the thread pool.
 * */
void serve()
{
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    listen(sockfd, 1024);

    while (1)
    {
        int client_fd = accept(sockfd, (struct sockaddr *) &client_addr, &client_addr_len);
        if (client_fd < 0)
        {
            printf("Foo: %s\n", inet_ntoa(client_addr.sin_addr));
            perror("Error accepting connection.");
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
 * off the waiting connections queue. It reads the command from the
 * connection and then sends the appropriate response.
 * */
void handle_connection(int conn_fd)
{
    size_t amt;
    char buffer[READ_BUFFER_SIZE] = {'\0'};
    char *curr_offset = buffer;
    while ((amt = read(conn_fd, buffer, READ_BUFFER_SIZE)) > 0)
    {
        curr_offset += amt - 1;
        if (is_eol(*curr_offset))
        {
            *curr_offset = '\0';
            curr_offset = buffer;
            command_handler *handler = parse_command(buffer, amt);
            handler(conn_fd);
            printf("\nHandled connection\n");
        }
    }
    printf("Closing connection\n");
    close(conn_fd);
}

int is_eol(char c)
{
    return c == '\0' || c == '\n' || c == '\r';
}

size_t read_command(int fd, char *buffer, size_t buffer_size)
{
    size_t total_read;
    size_t amt;
    while ((amt = read(fd, buffer, buffer_size) > 0))
    {
        total_read += amt;
        buffer += amt;
        buffer_size -= amt;
    }
    return total_read;
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
