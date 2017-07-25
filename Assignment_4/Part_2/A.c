#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include  <fcntl.h>
#define _GNU_SOURCE
#include <errno.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>
#include <signal.h>

# define BUFFER_SIZE 10

#define P(s) semop(s, &pop, 1)  /* pop is the structure we pass for doing
								   the P(s) operation (Wait) */
#define V(s) semop(s, &vop, 1)  /* vop is the structure we pass for doing
								   the V(s) operation (Signal) */

int main(){
	//for shared memory and semaphores
	int shmid,shmidHead,shmidTail,shmidInt,id,shmidStatus;
	int full,empty,mutex,val,*Int, *status;
	struct sembuf pop, vop ;
	key_t key0,key1,key2,key3,key4,key5,key6,key7; //keys are used to connect semaphores and shared memory across processes

	//the id for every key is different 
	key0=ftok(".",1);
	if(key0==-1) {
		perror("ERROR ");
		return 0;
	}
	key1=ftok(".",2);
	if(key1==-1) {
		perror("ERROR ");
		return 0;
	}
	key2=ftok(".",3);
	if(key1==-1) {
		perror("ERROR ");
		return 0;
	}
	key3=ftok(".",4);
	if(key1==-1) {
		perror("ERROR ");
		return 0;
	}
	key4=ftok(".",5);
	if(key1==-1) {
		perror("ERROR ");
		return 0;
	}
	key5=ftok(".",6);
	if(key1==-1) {
		perror("ERROR ");
		return 0;
	}
	key6=ftok(".",7);
	if(key6==-1) {
		perror("ERROR ");
		return 0;
	}
	key7=ftok(".",8);
	if(key7==-1) {
		perror("ERROR ");
		return 0;
	}
	/*
	    The operating system keeps track of the set of shared memory
	    segments. In order to acquire shared memory, we must first
	    request the shared memory from the OS using the shmget()
        system call. The second parameter specifies the number of
	    bytes of memory requested. shmget() returns a shared memory
	    identifier (SHMID) which is an integer. Refer to the online
	    man pages for details on the other two parameters of shmget()
	*/

	//this is for telling A that B (server) has been created
	//the semaphores are initialised in B and also destroyed in B
	shmidStatus = shmget(key7, 1*sizeof(int), 0777|IPC_CREAT);
	if (shmidStatus==-1) perror("shmget ERROR ");						//in case of error

	status = (int *) shmat(shmidStatus, 0, 0);
	if (*status==-1) perror("shmat ERROR ");							//in case of error
	//Waiting till the time B has created and initialised the necessary semaphores and shared memory after which it changes the status value to 1
	while(status[0]!=1) ;
	//Now B has initialised the semaphores and this process has acknowledged it and is safe to proceed from here

	//creating shared memories for array buffer of size BUFFER_SIZE, head pointer, tail pointer and Int for interrupt status form B
	shmid = shmget(key0, BUFFER_SIZE*sizeof(int), 0777|IPC_CREAT);
	if (shmid==-1) perror("shmget ERROR ");								//in case of error
	shmidHead = shmget(key1, 1*sizeof(int), 0777|IPC_CREAT);
	if (shmidHead==-1) perror("shmget ERROR ");							//in case of error
	shmidTail = shmget(key2, 1*sizeof(int), 0777|IPC_CREAT);
	if (shmidTail==-1) perror("shmget ERROR ");							//in case of error
	shmidInt = shmget(key3, 1*sizeof(int), 0777|IPC_CREAT);
	if (shmidInt==-1) perror("shmget ERROR ");							//in case of error
	
	/* In the following  system calls, the second parameter indicates the
	   number of semaphores under this semid. Throughout this lab,
	   give this parameter as 1. If we require more semaphores, we
	   will take them under different semids through separate semget()
	   calls.
	*/

	//Creating three semaphores, one for mutex (to ensure mutual exclusion) so that only one process can modify (write into or modify head/tail pointers) the shared circular buffer
	//one for keeping a count of number of places in the buffer that are currently full
	//one for keeping a count of number of places in the buffer that are currently empty
	full = semget(key4, 1, 0777|IPC_CREAT);
	if (full == -1) perror("semget ERROR ");							//in case of error
	empty = semget(key5, 1, 0777|IPC_CREAT);
	if (empty == -1) perror("semget ERROR ");							//in case of error
	mutex = semget(key6, 1, 0777|IPC_CREAT);
	if (mutex == -1) perror("semget ERROR ");							//in case of error

	/*  We now initialize the sembufs pop and vop so that pop is used
	    for P(semid) and vop is used for V(semid). For the fields
	    sem_num and sem_flg refer to the system manual. The third
	    field, namely sem_op indicates the value which should be added
	    to the semaphore when the semop() system call is made. Going
	    by the semantics of the P and V operations, we see that
	    pop.sem_op should be -1 and vop.sem_op should be 1.
	*/
	pop.sem_num = vop.sem_num = 0;
	pop.sem_flg = vop.sem_flg = 0;
	pop.sem_op = -1 ; vop.sem_op = 1 ;

	Int=(int *)shmat(shmidInt,0,0);	//for telling A that B has been interrupted
	int *head,*tail,*a,i=0;
	//attaching local variables to the shared memory
	a = (int *) shmat(shmid, 0, 0);
	if (*a==-1) perror("shmat ERROR ");					//in case of error
	head = (int *) shmat(shmidHead, 0, 0);
	if (*head==-1) perror("shmat ERROR ");				//in case of error
	tail = (int *) shmat(shmidTail, 0, 0);
	if (*tail==-1) perror("shmat ERROR ");				//in case of error

	while(1) {
		int sleep_time;

		// sleep for a random time (1 or 2 seconds) to test that
		// the waiting thread actually waits
		sleep_time = (rand() % 3);
		sleep(sleep_time);

		//generating random number between -5 and 5
		i=-5+rand()%11;

		//Waiting for an empty postition in the circular buffer
		P(empty);
		P(mutex);											//Wait for any other process which could modify the shared circular buffer or its head
															//or tail pointers

		//Producer after ensuring no other process enters this segment writes into the circular buffer as per the current iteration of the producer process
		a[head[0]]=i;
		head[0]=head[0]+1;       							//incrementing the head pointer for the shared buffer which defines the last write place in the buffer to ensure no other producer process writes into it
		if(head[0]==BUFFER_SIZE) head[0]=0;     			//if head reaches the end of array, head is made = 0

		printf("Passing requset %d \n",i);					//for debugging purposes
		//Allowing other process to pass across P(mutex)
		V(mutex);
		V(full);											//Incrementing the number of full places by one

		//if B is interrupeted, it waits for 2 seconds before destroying all the semaphores and shared memory
		if(Int[0]==1) break;
	}

	//detaching the local variables from the shared variables
	val=shmdt(Int);
	if (val == -1) perror("semdt ERROR ");				//in case of error
	val=shmdt(a);
	if (val == -1) perror("semdt ERROR ");				//in case of error
	val=shmdt(head);
	if (val == -1) perror("semdt ERROR ");				//in case of error
	val=shmdt(tail);
	if (val == -1) perror("semdt ERROR ");				//in case of error
	val=shmdt(status);
	if (val == -1) perror("semdt ERROR ");				//in case of error

	return 0;
}