/*---------------------------------------------------------------------------------------
--	SOURCE FILE:		server.cpp
--
--	PROGRAM:			server
--
--	FUNCTIONS:          main()
--                      server()
--                      remove_ipc (int signo)
--                      
--	DATE:				March 4, 2020
--
--	REVISIONS:			(Date and Description)
--
--	DESIGNERS:          Justin Cervantes
--
--	PROGRAMMERS:		Justin Cervantes
--
--	NOTES:
--	Launches an infinite loop which watches for messages on a message queue with type 1.
--  If such a message is seen on the queue, it spawns a child process which processes
--  the file that was passed in the message and sends it fragments in the size specified.
--  A semaphore is used to guarantee that each thread is getting proportional slots in the
--  message queue. The process is a slave to the client, and feeds the client until the
--  whole text file is read back to them. Entering ctrl + c will execute a signal which
--  has been tasked to close the program and also destroy all semaphores and message
--  queues.
--
--
--  COMPILE:
--  g++ -Wall -o server server.cpp
--
--  RUN:
--  ./server
---------------------------------------------------------------------------------------*/

#include "msg.h"

// Function Prototypes
void server(int queue, int type, int pid, int size, char message[MAXMESSAGEDATA]);
void remove_ipc (int signo);

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: main
--
-- DATE: March 4, 2020
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Justin Cervantes
--
-- PROGRAMMER: Justin Cervantes
--
-- INTERFACE: void main()
--
-- RETURNS:
-- N/A
--
-- NOTES:
-- This function does the following:
--    1. Create a signal handler to watch for a SIGINT signal (signal interrupt, ie ctrl + c) which will delete the IPCS
--    2. In a loop, Make a blocking call that reads the message queue looking for priority 1 messages 
--       (requests to join server)
--    3. If one is received, fork a child process that calls the server method and continue
----------------------------------------------------------------------------------------------------------------------*/
int main() {

    // Allocate one resource to the shared semaphore to start the reading, and one to allow constant checking
    // for message type 1
    V(sid);
    V(sid);
    
	// Set up the signal handler to remove the IPC objects on SIGINT received (ie ctrl + c)
    struct sigaction act;
    act.sa_handler = remove_ipc;
    act.sa_flags = 0;
    if ((sigemptyset (&act.sa_mask) == -1 || sigaction (SIGINT, &act, NULL) == -1))
    {
            perror ("Failed to set SIGINT handler");
            exit (EXIT_FAILURE);
    }

    // Setup general purpose message struct
    Mesg * mymsg = (Mesg*) malloc(sizeof(Mesg));

    while(1) {

        // Read for client joining requests (mtype of 1)
        P(sid);
        mesg_recv(queue, mymsg, 1, IPC_NOWAIT);
        V(sid);

        if(mymsg->mesg_type == 1) {
            
            // Notify the server terminal of the connection of a new client
            cout << "Message type: " << mymsg->mesg_type << endl;
            cout << "Message PID: " << mymsg->pid << endl;
            cout << "Message size for relay: " << mymsg->mesg_len << endl;
            cout << "Message data: " << mymsg->mesg_data << endl;

            fflush(stdout);
            
            // Give a non-shared version of the message to the child
            if(fork() == 0) {
                printf("Server has been forked.\n\n");
                server(queue, mymsg->mesg_type, mymsg->pid, mymsg->mesg_len, mymsg->mesg_data);
            }

            // Reset the message type to a non-one number so you don't end up in a continuous loop
            mymsg->mesg_type = 0;

        }

    }   

    return 0;

}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: server
--
-- DATE: March 4, 2020
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Justin Cervantes
--
-- PROGRAMMER: Justin Cervantes
--
-- INTERFACE: void server(int queue, int type, int pid, int size, char message[32]) 
--
-- RETURNS:
-- N/A
--
-- NOTES:
-- This function does the following:
--    1. Attempts to opent he requested file  using an ifstream. If it fails, send a message back to the client
--       and destroy the process by exiting
--    2. In a loop, constantly reset the values of the message to send, then send out number of chars from the requested
--       file. The semaphore which locks it should synchronize the results of what gets put on the queue and what gets read.
--
-- CRITICAL:
-- strcpy has the potential to overflow the struct's buffer. It is VERY important the user does not enter a value greater
-- than 32.
----------------------------------------------------------------------------------------------------------------------*/
void server(int queue, int type, int pid, int size, char message[32]) {

    // Setup general purpose message struct
    Mesg * mymsg = (Mesg*) malloc(sizeof(Mesg));


    // Build the file from the filename
    std::ifstream ifs (message, std::ifstream::in);
    char * buffer = new char[32];
    
    // Send a standard ascii error message to the user over the pipe
    if(!ifs.is_open()) {
        
        mymsg->mesg_type = 2;
        mymsg->pid = 0;
        mymsg->mesg_len = 26;
        char err_buff[26] = "The file failed to open!";
        strcpy(mymsg->mesg_data, err_buff);

        cout << "Exiting process and sending fail message: " << mymsg->mesg_data << endl;
        
        P(sid);
        mesg_send(queue, mymsg);
        V(sid);

        free(mymsg);
        exit(1);
    }

    while(1) {

        // Reset the tmp message for sending to the client's PID
        mymsg->mesg_type = pid;
        mymsg->pid = 0;
        mymsg->mesg_len = size;


        // Continues once a message is available on the server_queue 
        if( ifs.read(buffer, size) ) {

            // Put the buffer contents into the message for delivery
            strcpy(mymsg->mesg_data, buffer);
    
            P(sid);
            mesg_send(queue, mymsg);
            V(sid);

        } else {
            cout << "End of file - terminating child-slave process: " << pid << endl;
            exit(1);
        }

    }

}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: remove_ipc
--
-- DATE: March 4, 2020
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Aman Abdulla
--
-- PROGRAMMER: Aman Abdulla
--
-- INTERFACE: void remove_ipc (int signo)
--
-- RETURNS:
-- N/A
--
-- NOTES:
-- This function does the following:
--    1. Delete all message queues and delete all semaphores
--    2. Send out an error message if the actions failed.
----------------------------------------------------------------------------------------------------------------------*/
void remove_ipc (int signo)
{
    if (msgctl (queue, IPC_RMID, 0) < 0)
            perror ("msgctl");
    if (semctl (sid, 0, IPC_RMID, 0) < 0)
            perror ("semctl");
    exit(1);
}
