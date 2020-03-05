#include <sys/msg.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdlib.h> 
#include <unistd.h>
#include <iostream> 
#include <fstream>
#include <vector> 
#include <signal.h>
#include <thread>

#define MQ_KEY 1234L
#define MAXMESSAGEDATA 32
#define MESGHDRSIZE (sizeof(Mesg) - MAXMESSAGEDATA)
#define sem_key 200

using namespace std;

typedef struct {
    long mesg_type;
    int mesg_len;
    int pid;
    char mesg_data[MAXMESSAGEDATA];
} Mesg;


void P(int sid)     /* acquire semophore */
{
  struct sembuf *sembuf_ptr;
      
  sembuf_ptr= (struct sembuf *) malloc (sizeof (struct sembuf *) );
  sembuf_ptr->sem_num = 0;
  sembuf_ptr->sem_op = -1;
  sembuf_ptr->sem_flg = SEM_UNDO;
  if ((semop(sid,sembuf_ptr,1)) == -1)
  printf("semop error\n");
}

void V(int sid)     /* release semaphore */
{
	struct sembuf *sembuf_ptr;
  sembuf_ptr= (struct sembuf *) malloc (sizeof (struct sembuf *) );
  sembuf_ptr->sem_num = 0;
  sembuf_ptr->sem_op = 1;
  sembuf_ptr->sem_flg = SEM_UNDO;
  if ((semop(sid,sembuf_ptr,1)) == -1)
  printf("semop error\n");
}

int initsem (key_t key)
{
  int sid, status=0;

  if ((sid = semget((key_t)key, 1, 0666|IPC_CREAT|IPC_EXCL)) == -1)
  {
    if (errno == EEXIST)
      sid = semget ((key_t)key, 1, 0);
  }
  else   /* if created */
      status = semctl (sid, 0, SETVAL, 0);
  if ((sid == -1) || status == -1)
  {
    perror ("initsem failed\n");
    return (-1);
  }
  else
    return (sid);
}

int mesg_send(int queue, Mesg * mymsg) {
  return (msgsnd(queue, mymsg, sizeof(Mesg) - sizeof(long), 0));
}

int mesg_recv(int queue, Mesg * mymsg, int type, int wait) {
  return (msgrcv(queue, mymsg, sizeof(Mesg) - sizeof(long), type, wait));
}


// IPC: Set up client-facing message queue
int queue = msgget(MQ_KEY, IPC_CREAT);

// IPC: Setup shared semaphore
int sid = initsem ((key_t)sem_key);

