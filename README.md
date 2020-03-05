# MQ-Multiplexer

The objective of the MQ Multiplexer is to demonstrate that you can set priority by reading a file at different speeds using a single IPC resource. In this particular application, this is done by ensuring that an equally proportional number of messages for each client are put on the message queue and manipulating the size of the message for each client based on priority (priority is set by how much data to get from each message). This is managed with the use of a semaphore: when one process interfaces with the message queue, all other processes line up to use it, and so the process currently interfacing will have to wait for everyone in the line to go before trying to get access to the queue again. Priority is encapsulated in the fact that different messages have different sizes on the queue (though equal proportions). Higher priority messages therefore transmit more data through the IPC.

## To compile
make server
make client

## To run
./server
./client

# Overview of Design

## Server
![](/Documentation/Images/server.png)

## Server Child Processes
![](/Documentation/Images/server_child_processes.png)

## Client
![](/Documentation/Images/client.png)


# Pseudocode

## Server

### main
Create a signal handler to watch for a SIGINT signal (signal interrupt, ie ctrl+c) which will delete the IPCS
In a loop, makes a non-blocking read on the message queue looking for priority 1 messages (requests to
join the server). These requests should be captured in a semaphore to ensure that it isn’t starved out of
getting onto the message queue due to competition from the client processes.
If a request to join is received, fork a child process that calls the server method and continues to loop
looking for more type 1 messages.

### server
Read the message contained from the parent process and extract the file name that it wants to have
read back to it.
Attempt to open the file using an ifstream; if it fails to open, send a message (guarded by a semaphore
to make sure it has the opportunity to line up for the resource) which lets the client know an error
occurred and that it can terminate. Use the pid of the client to send messages to it via the message type.
Terminate yourself afterwards.
If the ifstream creation is successful, in a loop, constantly reset the message values and send a message
with the characters from the text file specified (the number of chars to specified will be passed to you
from the parent process).
If the end of the file is reached, terminate yourself after letting the user know the text file was
successfully read.

## Client

### main
Launch a thread which makes an initial request to join the server.
In a loop, check for message types which are either 2 (received immediately after a file open fails). This
should be guarded in a semaphore.
If a 2 message type is detected, terminate yourself.
Make another non-blocking read which is guarded in a semaphore and look for messages with a
message type that matches your PID. Echo out the contents once you receive something.

### Client (thread which requests to join the server)
Get user input on your priority (number from 1 – 32). This signifies the amount of bytes to get.
Get user input for what text file to open.
Send a message with this information along with your PID with a message type of 1. If message send
fails, inform client and terminate.
