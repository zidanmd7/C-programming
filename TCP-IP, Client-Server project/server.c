/*
* Auth: Zidan Mohammed
 * Date: 04-27-26  (Due:04-29-26)
 * Course: CSCI-3550 (Sec: 001)
 * Desc:  Project 1 (Client-Server File Transfer with TCP/IP).
 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUF_SIZE (10 * 1024 * 1024)

char *buffer = NULL;
int listen_sockfd = -1;
int client_sockfd = -1;
int outfd = -1;

void cleanup(void);
void SIGINT_handler(int sig);
int check_port(int port);
int setup_server(int port);
int receive_file(size_t *total_bytes);
int save_file(char *filename, size_t total_bytes);

void cleanup(void) {
    if (outfd != -1) {
        close(outfd);
        outfd = -1;
    }

    if (client_sockfd != -1) {
        close(client_sockfd);
        client_sockfd = -1;
    }

    if (listen_sockfd != -1) {
        close(listen_sockfd);
        listen_sockfd = -1;
    }

    if (buffer != NULL) {
        free(buffer);
        buffer = NULL;
    }
}

/* SIGINT handler for the client */
void SIGINT_handler( int sig ) {

    /* Issue an error */
    fprintf( stderr, "server: Server interrupted. Shutting down.\n" );

    /* Cleanup after yourself */
    cleanup();

    /* Exit for 'reals' */
    exit( EXIT_FAILURE );

} /* end SIGINT_handler() */



/* makes sure the server cannot listen to priveleged ports */
int check_port(int port) {
    if (port >= 0 && port <= 1023) {
        fprintf(stderr, "server: ERROR: Port number is privileged.\n");
        return -1;
    }

    if (port > 65535) {
        fprintf(stderr, "server: ERROR: Port number is invalid.\n");
        return -1;
    }

    return 0;
}



/* here we create the listening port and then bind it to 127.0.0.1 and the given port, and then listen for TCP connection*/
int setup_server(int port) {
    struct sockaddr_in server_addr;
    int val;

    listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (listen_sockfd < 0) {
        fprintf(stderr, "server: ERROR: Failed to create socket.\n");
        return -1;
    }

    val = 1;

    if (setsockopt(listen_sockfd,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   (const void *) &val,
                   sizeof(int)) != 0) {
        fprintf(stderr, "server: ERROR: setsockopt() failed.\n");
        return -1;
    }

    memset((void *) &server_addr, 0, sizeof(server_addr));


    /* hard code the server to 127.0.0.1 */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((unsigned short int) port);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(listen_sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
        fprintf(stderr, "server: ERROR: Failed to bind socket.\n");
        return -1;
    }

    /* listening socket */
    if (listen(listen_sockfd, 32) != 0) {
        fprintf(stderr, "server: ERROR: listen(): Failed.\n");
        return -1;
    }

    return 0;
}

int receive_file(size_t *total_bytes) {
    ssize_t bytes_read;
    size_t total;
    size_t space_left;

    total = 0;
    space_left = BUF_SIZE;

    bytes_read = recv(client_sockfd, (void *) (buffer + total), space_left, 0);

    while (bytes_read > 0) {
        total = total + (size_t) bytes_read;

        if (total >= BUF_SIZE) {
            break;
        }

        space_left = BUF_SIZE - total;

        bytes_read = recv(client_sockfd, (void *) (buffer + total), space_left, 0);
    }

    if (bytes_read < 0) {
        fprintf(stderr, "server: ERROR: Reading from socket.\n");
        return -1;
    }

    *total_bytes = total;

    return 0;
}


/* create or overwrites the file and writes the received bytes to disk */
int save_file(char *filename, size_t total_bytes) {
    ssize_t bytes_written;
    size_t total_written;

    outfd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (outfd == -1) {
        fprintf(stderr, "server: ERROR: Unable to create: %s\n", filename);
        return -1;
    }

    total_written = 0;

    while (total_written < total_bytes) {
        bytes_written = write(outfd,
                              (const void *) (buffer + total_written),
                              total_bytes - total_written);

        if (bytes_written < 0) {
            fprintf(stderr, "server: ERROR: Unable to write: %s\n", filename);
            return -1;
        }

        total_written = total_written + (size_t) bytes_written;
    }

    close(outfd);
    outfd = -1;

    return 0;
}

int main(int argc, char *argv[]) {
    int port;
    int file_number;
    size_t total_bytes;
    char filename[32];

    signal(SIGINT, SIGINT_handler);

    if (argc != 2) {
        fprintf(stderr, "server: USAGE: server <listen_Port>\n");
        return EXIT_FAILURE;
    }

    port = atoi(argv[1]);

    if (check_port(port) != 0) {
        return EXIT_FAILURE;
    }

    buffer = (char *) malloc(BUF_SIZE);

    if (buffer == NULL) {
        fprintf(stderr, "server: ERROR: Failed to allocate memory.\n");
        return EXIT_FAILURE;
    }

    if (setup_server(port) != 0) {
        cleanup();
        return EXIT_FAILURE;
    }

    file_number = 1;

    /* file numbering */
    while (1) {
        printf("server: Awaiting TCP connections over port %d...\n", port);

        client_sockfd = accept(listen_sockfd, NULL, NULL);

        if (client_sockfd < 0) {
            fprintf(stderr, "server: ERROR: While attempting to accept a connection.\n");
            cleanup();
            return EXIT_FAILURE;
        }

        printf("server: Connection accepted!\n");
        printf("server: Receiving file...\n");

        if (receive_file(&total_bytes) != 0) {
            cleanup();
            return EXIT_FAILURE;
        }

        if (client_sockfd != -1) {
            close(client_sockfd);
            client_sockfd = -1;
        }

        printf("server: Connection closed.\n");

        sprintf(filename, "file-%02d.dat", file_number);

        printf("server: Saving file: \"%s\"...\n", filename);

        if (save_file(filename, total_bytes) != 0) {
            cleanup();
            return EXIT_FAILURE;
        }

        printf("server: Done.\n\n");

        file_number++;
    }

    cleanup();

    return EXIT_SUCCESS;
}
