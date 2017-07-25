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

//structure to store our id of each threads and x that is the request value from process X's producer queue it is passed as the arguement to the thread_start function
typedef struct t {
	int x;
} thread_data;

//Int is the shared variable to tell process A that B has been interrupted after which A deletes the semaphores and shared memory
//activeCount is the number of threads started but not finished
//sum is used for debugging purpose it stores the sum of valid requests from A and is then compared with the current value of Tickets
int *Int,Tickets=100,activeCount=0,sum=0,blocked=0;//blocked is for status of the main B thread, which gets blocked on overloading
pthread_mutex_t cnt_mutex;	//one mutex for threads in B
pthread_cond_t cnt_cond;	//one cond for synchronization between threads in B

//defined here so that the shared memory and semaphores can be destroyed in the sigint handler function
//for shared memory and semaphores
int shmid,shmidHead,shmidTail,shmidInt,id,shmidStatus;
int full,empty,mutex,val, *status;
int *head,*tail,*a,x=0,i=0;
struct sembuf pop, vop ;
key_t key0,key1,key2,key3,key4,key5,key6,key7; //keys are used to connect semaphores and shared memory across processes


//incase ctrl+c is pressed
void my_handler(int signum)
{	
	Int[0]=1;

	//wait for 2 seconds, then delete the semaphores and shared memory
	//waiting is necessary for A processes to acknowledge change in Int shared memory and then exit themselves
	printf("Destroying semaphores and shared memories\n");
	sleep(2);

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

	//deleting the semaphores
	val=semctl(full, 0, IPC_RMID, 0);
	if (val == -1) perror("semctl ERROR ");							//in case of error
	val=semctl(empty, 0, IPC_RMID, 0);
	if (val == -1) perror("semctl ERROR ");							//in case of error
	val=semctl(mutex, 0, IPC_RMID, 0);
	if (val == -1) perror("semctl ERROR ");							//in case of error

	//removing the shared memories
	val=shmctl(shmid, IPC_RMID, 0);
	if (val == -1) perror("semctl ERROR ");							//in case of error
	val=shmctl(shmidHead, IPC_RMID, 0);
	if (val == -1) perror("semctl ERROR ");							//in case of error
	val=shmctl(shmidTail, IPC_RMID, 0);
	if (val == -1) perror("semctl ERROR ");							//in case of error
	val=shmctl(shmidStatus, IPC_RMID, 0);
	if (val == -1) perror("semctl ERROR ");							//in case of error
	val=shmctl(shmidInt, IPC_RMID, 0);
	if (val == -1) perror("semctl ERROR ");							//in case of error

	// clean up the mutex and condition variable
	val=pthread_mutex_destroy(&cnt_mutex);
	if (val!=0) {
		perror("mutex destroy ");
		exit(0);
	}
	val=pthread_cond_destroy(&cnt_cond);
	if (val!=0) {
		perror("cond destroy ERROR ");
		exit(0);
	}
	//exit process B
	exit(0);
}


void *book_ticket(void *param)		//this fuction is called with each new thread created
{
	int val;
	thread_data *t_param = (thread_data *) param;
	int x = (*t_param).x;			//The request from process A
	int sleep_time;					//for making the thread sleep

	val=pthread_mutex_lock(&cnt_mutex);	//locking the mutex so only one active thread can enter this critical section at a time
	if (val!=0) {					//in case of Error
		perror("mutex lock ERROR ");
		exit(0);
	}

	//increasing the number of active worker threads
	activeCount++;	

	//make the thread sleep between 0 and 2 seconds assuming both inclusive
	sleep_time=rand()%3;

	//if request is a valid one
	if( Tickets-x >=0 ) {
		if (Tickets-x <= 100) {			//decrement Tickets
			Tickets=Tickets-x;
			sum+=x;
		}
		else {							//Ticket not changed
			sum=0;
			Tickets=100;
		}

		val=pthread_mutex_unlock(&cnt_mutex);	//Unlocking the mutex so that other thread can enter the Critical Section and process their request before this thread goes to sleep
		if (val!=0) {
			perror("mutex unlock ERROR ");
			exit(0);
		}

		//making the thread sleep
		sleep(sleep_time);

		//locking the thread to decrement activeCount and print the value of Tickets
		val=pthread_mutex_lock(&cnt_mutex);	//locking the mutex so only one active thread can enter this critical section at a time
		if (val!=0) {
			perror("mutex lock ERROR ");
			exit(0);
		}

		//printing the Tickets, in assignment it was mentioned to print it after sleep
		printf("The value of Tickets is : %d\n",Tickets);

		//this thread will end so decrementing activeCount
		activeCount--;

		//we check if main thread is blocked, then only we signal, signaling a non waiting process can generate undefined behaviour
		if(activeCount <= 5 && blocked == 1) {			//signal other waiting threads to resume operation using cnt_cond, signal is used to allow only one thread to resume otherwise activeCount may be more than 10
			val=pthread_cond_signal(&cnt_cond);			// signal to the waiting processes if the condition is successful.
			if (val!=0) {
				perror("cond broadcast ERROR ");
				exit(0);
			}
		}

		val=pthread_mutex_unlock(&cnt_mutex);	//Unlocking the mutex so that other thread can enter the Critical Section and process their request before this thread exits
		if (val!=0) {
			perror("mutex unlock ERROR ");
			exit(0);
		}

		pthread_exit(1); //exit the threads with status 1
	}

	else {
		val=pthread_mutex_unlock(&cnt_mutex);	//Unlocking the mutex so that other thread can enter the Critical Section and process their request before this thread goes to sleep
		if (val!=0) {
			perror("mutex unlock ERROR ");
			exit(0);
		}

		//making the thread sleep
		sleep(sleep_time);
	
		//locking the thread to decrement activeCount and print the value of Tickets
		val=pthread_mutex_lock(&cnt_mutex);	//locking the mutex so only one active thread can enter this critical section at a time
		if (val!=0) {
			perror("mutex lock ERROR ");
			exit(0);
		}

		//printing the message that request cant be fulfilled, in assignment it was mentioned to print it after sleep
		printf("Request cannot be fulfilled\n");

		//this thread will end so decrementing activeCount
		activeCount--;

		//we check if main thread is blocked, then only we signal, signaling a non waiting process can generate undefined behaviour
		if(activeCount <= 5 && blocked == 1) {			//signal other waiting thread to resume operation using cnt_cond, signal is used to allow only one thread to resume otherwise activeCount may be more than 10
			val=pthread_cond_signal(&cnt_cond);			// signal to the waiting processes if the condition is successful.
			if (val!=0) {
				perror("cond broadcast ERROR ");
				exit(0);
			}
		}

		val=pthread_mutex_unlock(&cnt_mutex);	//Unlocking the mutex so that other thread can enter the Critical Section and process their request before this thread exits
		if (val!=0) {
			perror("mutex unlock ERROR ");
			exit(0);
		}

		pthread_exit(0); //exit the thread with status 0
	}
	
}

int main(){
	
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

	//creating shared memories for array buffer of size BUFFER_SIZE, head pointer, tail pointer and SUM
	shmid = shmget(key0, BUFFER_SIZE*sizeof(int), 0777|IPC_CREAT);
	if (shmid==-1) perror("shmget ERROR ");								//in case of error
	shmidHead = shmget(key1, 1*sizeof(int), 0777|IPC_CREAT);
	if (shmidHead==-1) perror("shmget ERROR ");							//in case of error
	shmidTail = shmget(key2, 1*sizeof(int), 0777|IPC_CREAT);
	if (shmidTail==-1) perror("shmget ERROR ");							//in case of error
	shmidInt = shmget(key3, 1*sizeof(int), 0777|IPC_CREAT);
	if (shmidInt==-1) perror("shmget ERROR ");							//in case of error
	shmidStatus = shmget(key7, 1*sizeof(int), 0777|IPC_CREAT);
	if (shmidStatus==-1) perror("shmget ERROR ");						//in case of error

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

	/* The following system calls sets the values of the semaphores
	   mutex to 1, full to 0, empty to BUFFER_SIZE*/
	val=semctl(full, 0, SETVAL, 0);
	if (val == -1) perror("semctl ERROR ");								//in case of error
	val=semctl(empty, 0, SETVAL, BUFFER_SIZE);
	if (val == -1) perror("semctl ERROR ");								//in case of error
	val=semctl(mutex, 0, SETVAL, 1);
	if (val == -1) perror("semctl ERROR ");								//in case of error

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

	//attaching local variables to the shared memory
	a = (int *) shmat(shmid, 0, 0);
	if (*a==-1) perror("shmat ERROR ");					//in case of error
	head = (int *) shmat(shmidHead, 0, 0);
	if (*head==-1) perror("shmat ERROR ");				//in case of error
	tail = (int *) shmat(shmidTail, 0, 0);
	if (*tail==-1) perror("shmat ERROR ");				//in case of error
	status = (int *) shmat(shmidStatus, 0, 0);
	if (*status==-1) perror("shmat ERROR ");			//in case of error



	// register signal handler fo Ctrl-C
	signal(SIGINT, my_handler);

	//initialising mutex and condition variables
	val=pthread_mutex_init(&cnt_mutex, NULL);
	if (val!=0) {
		perror("mutex init ERROR ");
		exit(0);
	}
	val=pthread_cond_init(&cnt_cond, NULL);		//for synchronisation between threads
	if (val!=0) {
		perror("cond init ERROR ");
		exit(0);
	}

	//Indicating process A that everything has been initialised, it can begin sending requests in the shared buffer
	status[0]=1;

	//here we take requests from A till ctrl+C is pressed upon which signal handler is called
	while(1) {
		pthread_t tid;
		x=-10;
		
		//for checking if new requests can be taken up by checking the number of active threads
		val=pthread_mutex_lock(&cnt_mutex);	//locking the mutex so only one active thread can enter this critical section at a time
		if (val!=0) {
			perror("mutex lock ERROR ");
			exit(0);
		}
		while (activeCount > 10) {			//more than 10 threads active so this process is made to wait till the active threads become less than 6 again
			blocked=1;						//to indicate that the main thread is blocked
			val=pthread_cond_wait(&cnt_cond, &cnt_mutex);//if more than 10 worker threads, block the thread, unlock mutex till condition is met
			if (val!=0) {
				perror("cond wait ERROR ");
				exit(0);
			}
		}

		//main thread not blocked, so reseting the status to 0
		blocked=0;

		val=pthread_mutex_unlock(&cnt_mutex);	//unlocking the mutex so other threads can enter critical section
		if (val!=0) {
			perror("mutex lock ERROR ");
			exit(0);
		}

		//time to process a request and assign a thread to the request
		//Waiting for a full postition in the circular buffer
		P(full);
		P(mutex);												//Wait for any other process which could modify the shared circular buffer or its head
																//or tail pointers

		//Consumer after ensuring no other process enters this segment reads from the circular buffer in an infinite loop which breaks if the value of SUM is as desired according to m
		x=a[tail[0]];											//x stores the request number
		tail[0]=tail[0]+1;										//incrementing the tail pointer for the shared buffer which defines the last read place in the buffer to ensure no other consumer process reads it
		if(tail[0]==BUFFER_SIZE) tail[0]=0;						//if tail reaches the end of array, head is made = 0

		thread_data param;
		param.x=x;												//to pass the request number to the book_ticket function on creation of new thread
		i++;
		pthread_create(&tid, NULL, book_ticket, (void *) &param);	//creating a new thread and calling book_ticket function with param arguement

		//Allowing other process to pass across P(mutex)
		V(mutex);
		V(empty);												//Incrementing the number of empty
		
	}
	return 0;
}