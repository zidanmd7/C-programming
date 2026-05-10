# Client-Server File Transfer with TCP/IP

## Overview
This project is a simple client-server file transfer program written in C. 
It uses TCP/IP sockets to send files from a client program to a server program.

The server waits for incoming TCP connections on a given port. 
The client connects to the server using the server IP address and port number, then sends one or more files. 
Each file received by the server is saved using a numbered filename such as `file-01.dat`, `file-02.dat`, and so on.


## Files
File | Description
`server.c` | Starts the server, listens for client connections, receives files, and saves them locally.
`client.c` | Connects to the server and sends one or more files to it.


## How It Works
The server creates a TCP socket and listens on `127.0.0.1` using the port number provided by the user. 
When a client connects, the server receives the file data, closes the connection, and saves the file with an automatically generated name.

The client takes the server IP address, port number, and file names as command-line arguments. 
It connects to the server once for each file and sends the file contents over the TCP connection.

The program also includes basic error checking for invalid ports, socket creation, connection errors, file opening errors, and memory allocation errors. 
Both the client and server also handle `CTRL + C` using a signal handler so open files, sockets, and memory can be cleaned up before the program exits.


## Compile Instructions
Use `gcc` to compile both programs.

```bash
gcc -Wall -Wstrict-prototypes -Wmissing-prototypes -ansi -pedantic-errors -D_DEFAULT_SOURCE -o server server.c



## How to run
### 1. Start the server

Open a terminal and run:

```bash
./server 5000

##stop server using
CTRL + C


## 2. Simple first comment in a different terminal

./client <server_IP> <server_Port> file1 file2 ...
./client 127.0.0.1 5000 test1.txt test2.txt test3.txt
