/*---------------------------------------------------------------------------------------
--	SOURCE FILE:		client.cpp
--
--	PROGRAM:			client
--
--	FUNCTIONS:          main()
--                      client(int mtype)
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
--	Launches a redundant synchronous thread which makes initial contact with the server
--  by sending a type 1 message. The message contains in addition to type: name of a
--  text file to open for reading, the amount of characters it should of the file send per
--  message in the message queue (MAX 32!), and the client's PID, which will be used as the type
--  it is checking for in the queue. 
--
--  Any interface on the message queue is wrapped up in a shared semaphore with the server, 
--  a feature which ensures there are now concurrency issues (though since we're never 
--  reading from the front of the queue, this shouldn't happen anyway).
--
--  COMPILE:
--  g++ -Wall -lpthread -o client client.cpp
--
--  RUN:
--  ./client
---------------------------------------------------------------------------------------*/
#include "msg.h"

// Function prototypes
void client(int mtype);

// Globals
Mesg * mymsg = (Mesg*) malloc(sizeof(Mesg));

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: main()
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
--    1. Launch a thread (immediately joined) which calls client, wait for client to finish before continuing.
--    2. Makes a non-blocking read to see if there are any message type 2's on the queue, if so, file open failed. Exit.
--    3. If no type 2s, check for types with this process' PID - if it exists, read it out, if not, continue.
----------------------------------------------------------------------------------------------------------------------*/
int main () {

    thread pointlessthread(client, 1);
    pointlessthread.join();
    V(sid); 

    // Continuously read the client queue for any messages of the correct type (my pid)
    while(1) {

        // Check for a file open failure from server
        P(sid);
        int nread = mesg_recv(queue, mymsg, 2, IPC_NOWAIT);
        V(sid); 
        
        if(nread > 0) {
            cout << "Exiting due to file failure error!" << endl;
            fflush(stdout);
            exit(0);
        } else {
            nread = 0;
        }


                
        // Read the client queue in order (managed by shared client queue semapohre)
        P(sid);
        nread = mesg_recv(queue, mymsg, getpid(), IPC_NOWAIT);
        V(sid);

        // If data was received, print out the message's data
        if (nread > 0) {
            cout << mymsg->mesg_data;
            nread = 0;
        }
    }

    return 0;

}


/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: client(int mtype)
--
-- DATE: March 4, 2020
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Justin Cervantes
--
-- PROGRAMMER: Justin Cervantes
--
-- INTERFACE: void client(int mtype)
--
-- RETURNS:
-- N/A
--
-- NOTES:
-- This function does the following:
--    1. Set mtype to 1, set message attribute pid to this processes pid, get user input for chars to receive per message
--       and user input for a file name.
--    2. Sends the message with the information to the server over a message queue (shared key with server)
----------------------------------------------------------------------------------------------------------------------*/
void client(int mtype) {

    // Set the message type to 1 so the server prioritizes new client requests
    mymsg->mesg_type = mtype;
    
    // Set the client PID to the server so you know what to read in the queue
    mymsg->pid = getpid();

    // Get the amount of characters to print at a time
    cout << "Enter the number of characters to print at a time (less than 32):" << endl;
    int size;
    cin >> size;
    if(size > 32) {
        cout << "number must be less than 32" << endl;
        exit(1);
    }
    mymsg->mesg_len = size;
    
    // Clear the cin buffer as a residual newline is left from entering size
    cin.clear();
	cin.ignore(200, '\n');

    // Get the filepath from the user
    char input[MAXMESSAGEDATA];
    printf("Please provide a valid filename: \n");
    cin.getline(input, sizeof(input));
    printf("You have selected: %s\n", input);
    strcpy(mymsg->mesg_data, input);

    // Send the first message which requests to join the server
    int send_status = 1;
    P(sid);
    send_status = mesg_send(queue, mymsg);
    V(sid);
    if (send_status != 0) {
        cout << "Request to join failed, exiting program." << endl;
        exit(1);
    }
}