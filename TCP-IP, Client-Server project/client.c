/*
* Auth: Zidan Mohammed
 * Date: 04-28-26  (Due:04-29-26)
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
int sockfd = -1;
int filefd = -1;

void cleanup(void);
void SIGINT_handler(int sig);
int check_port(int port);
int connect_to_server(char *server_ip, int port);
int send_one_file(char *filename);


/* closes open file/socket and frees allocated memory.*/
void cleanup(void) {
    if (filefd != -1) {
        close(filefd);
        filefd = -1;
    }

    if (sockfd != -1) {
        close(sockfd);
        sockfd = -1;
    }

    if (buffer != NULL) {
        free(buffer);
        buffer = NULL;
    }
}

/* SIGINT handler for the client */
void SIGINT_handler( int sig ) {

    /* Issue an error */
    fprintf( stderr, "client: Client interrupted. Shutting down.\n" );

    /* Cleanup after yourself */
    cleanup();

    /* Exit for 'reals' */
    exit( EXIT_FAILURE );

} /* end SIGINT_handler() */



/* makes sure that privileged ports cant be used.*/
int check_port(int port) {
    if (port >= 0 && port <= 1023) {
        fprintf(stderr, "client: ERROR: Port number is privileged.\n");
        return -1;
    }

    if (port > 65535) {
        fprintf(stderr, "client: ERROR: Port number is invalid.\n");
        return -1;
    }

    return 0;
}


/* creates a TCP socket and connects it to the server IP/port */
int connect_to_server(char *server_ip, int port) {
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        fprintf(stderr, "client: ERROR: Failed to create socket.\n");
        return -1;
    }
    /* we clear the address structure before filling it */
    memset((void *) &server_addr, 0, sizeof(server_addr));


    /* htons() to convert the port into network byte order. inet_addr() to convert the IP string into an IPv4 address */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((unsigned short int) port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    /* server connection */

    if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
        fprintf(stderr, "client: ERROR: connecting to %s:%d\n", server_ip, port);
        return -1;
    }

    return 0;
}


/* opens one file then reads its bytes, and sends only those bytes to the server */
int send_one_file(char *filename) {
    ssize_t bytes_read;
    ssize_t bytes_sent;
    ssize_t total_sent;

    filefd = open(filename, O_RDONLY);

    if (filefd == -1) {
        fprintf(stderr, "client: ERROR: Failed to open: %s\n", filename);
        return 1;
    }

    bytes_read = read(filefd, buffer, BUF_SIZE);



    /* loop keeps sending until this chunk is fully sent */
    while (bytes_read > 0) {
        total_sent = 0;

        while (total_sent < bytes_read) {
            bytes_sent = send(sockfd,
                              (const void *) (buffer + total_sent),
                              (size_t) (bytes_read - total_sent),
                              0);

            if (bytes_sent < 0) {
                fprintf(stderr, "client: ERROR: While sending data.\n");
                return -1;
            }

            total_sent = total_sent + bytes_sent;
        }

        bytes_read = read(filefd, buffer, BUF_SIZE);
    }


    /* eturns negative if there was an error */
    if (bytes_read < 0) {
        fprintf(stderr, "client: ERROR: Unable to read: %s\n", filename);
        return -1;
    }

    close(filefd);
    filefd = -1;

    return 0;
}

int main(int argc, char *argv[]) {
    char *server_ip;
    int port;
    int i;
    int result;

    signal(SIGINT, SIGINT_handler);

    if (argc <= 3) {
        fprintf(stderr, "client: USAGE: client <server_IP> <server_Port> file1 file2 ...\n");
        return EXIT_FAILURE;
    }

    server_ip = argv[1];
    port = atoi(argv[2]);

    if (check_port(port) != 0) {
        return EXIT_FAILURE;
    }

    /* allocates one large buffer for file transfer */
    buffer = (char *) malloc(BUF_SIZE);

    if (buffer == NULL) {
        fprintf(stderr, "client: ERROR: Failed to allocate memory.\n");
        return EXIT_FAILURE;
    }


    /*
     * Start at argv[3] because:
     * argv[0] = program name
     * argv[1] = server IP
     * argv[2] = server port
     * argv[3] and after = file names
     */

    for (i = 3; i < argc; i++) {
        printf("client: Connecting to %s:%d...\n", server_ip, port);

        if (connect_to_server(server_ip, port) != 0) {
            cleanup();
            return EXIT_FAILURE;
        }

        printf("client: Success!\n");
        printf("client: Sending: \"%s\"...\n", argv[i]);

        result = send_one_file(argv[i]);


        /* if file could not open, it keeps going instead of terminating */
        if (result == 1) {
            if (sockfd != -1) {
                close(sockfd);
                sockfd = -1;
            }

            printf("\n");
            continue;
        }

        /* errors to terminate client */
        if (result == -1) {
            cleanup();
            return EXIT_FAILURE;
        }

        if (sockfd != -1) {
            close(sockfd);
            sockfd = -1;
        }

        printf("client: Done.\n\n");
    }

    printf("client: File transfer(s) complete.\n");
    printf("client: Goodbye!\n");

    cleanup();

    return EXIT_SUCCESS;
}
