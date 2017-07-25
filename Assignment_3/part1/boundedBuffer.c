#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>

//macros for constants used in the code as per the problem statement
# define BUFFER_SIZE 20
# define NUMBER_OF_ITERATIONS 50

#define P(s) semop(s, &pop, 1)  /* pop is the structure we pass for doing
								   the P(s) operation (Wait) */
#define V(s) semop(s, &vop, 1)  /* vop is the structure we pass for doing
								   the V(s) operation (Signal) */


//The  producer fuction which attaches the shared memory and then waits for an empty position in circular buffer
//It then waits for other processes to finish accessing the modifying the shared buffer before modifying it
//to ensure mutual exclusion of the processes whether it is producer or consumer process 
void producer (int i,int empty,int full, int mutex, int shmid,int shmidHead,int shmidTail, int shmidSum,struct sembuf pop,struct sembuf vop ){
	int *head,*tail,*sum,*a,val;
	//attaching local variables to the shared memory
	a = (int *) shmat(shmid, 0, 0);
	if (*a==-1) perror("shmat ERROR ");					//in case of error
	head = (int *) shmat(shmidHead, 0, 0);
	if (*head==-1) perror("shmat ERROR ");				//in case of error
	tail = (int *) shmat(shmidTail, 0, 0);
	if (*tail==-1) perror("shmat ERROR ");				//in case of error
	sum = (int *) shmat(shmidSum, 0, 0);
	if (*sum==-1) perror("shmat ERROR ");				//in case of error

	//Waiting for an empty postition in the circular buffer
	P(empty);
	P(mutex);											//Wait for any other process which could modify the shared circular buffer or its head
														//or tail pointers

	//Producer after ensuring no other process enters this segment writes into the circular buffer as per the current iteration of the producer process
	a[head[0]]=i;
	head[0]=head[0]+1;       							//incrementing the head pointer for the shared buffer which defines the last write place in the buffer to ensure no other producer process writes into it
	if(head[0]==BUFFER_SIZE) head[0]=0;     			//if head reaches the end of array, head is made = 0

	//Allowing other process to pass across P(mutex)
	V(mutex);
	V(full);											//Incrementing the number of full places by one

	//detaching the local variables from the shared variables
	val=shmdt(a);
	if (val == -1) perror("semdt ERROR ");				//in case of error
	val=shmdt(head);
	if (val == -1) perror("semdt ERROR ");				//in case of error
	val=shmdt(tail);
	if (val == -1) perror("semdt ERROR ");				//in case of error
	val=shmdt(sum);	
	if (val == -1) perror("semdt ERROR ");				//in case of error
	return;
}


//The  consumer fuction which attaches the shared memory and then waits for a full position in circular buffer
//It then waits for other processes to finish accessing the modifying the shared buffer before modifying it
//to ensure mutual exclusion of the processes whether it is producer or consumer process 
void consumer (int empty,int full, int mutex, int shmid,int shmidHead,int shmidTail, int shmidSum,struct sembuf pop,struct sembuf vop ){
	int *head,*tail,*sum,*a,val;
	//attaching local variables to the shared memory
	a = (int *) shmat(shmid, 0, 0);
	if (*a==-1) perror("shmat ERROR ");						//in case of error
	head = (int *) shmat(shmidHead, 0, 0);
	if (*head==-1) perror("shmat ERROR ");					//in case of error
	tail = (int *) shmat(shmidTail, 0, 0);
	if (*tail==-1) perror("shmat ERROR ");					//in case of error
	sum = (int *) shmat(shmidSum, 0, 0);	
	if (*sum==-1) perror("shmat ERROR ");					//in case of error

	//Waiting for a full postition in the circular buffer
	P(full);
	P(mutex);												//Wait for any other process which could modify the shared circular buffer or its head
															//or tail pointers

	//Consumer after ensuring no other process enters this segment reads from the circular buffer in an infinite loop which breaks if the value of SUM is as desired according to m
	sum[0]+=a[tail[0]];
	tail[0]=tail[0]+1;										//incrementing the tail pointer for the shared buffer which defines the last read place in the buffer to ensure no other consumer process reads it
	if(tail[0]==BUFFER_SIZE) tail[0]=0;						//if tail reaches the end of array, head is made = 0

	//Allowing other process to pass across P(mutex)
	V(mutex);
	V(empty);												//Incrementing the number of empty places by one

	//detaching the local variables from the shared variables
	val=shmdt(a);
	if (val == -1) perror("semdt ERROR ");					//in case of error
	val=shmdt(head);
	if (val == -1) perror("semdt ERROR ");					//in case of error
	val=shmdt(tail);
	if (val == -1) perror("semdt ERROR ");					//in case of error
	val=shmdt(sum);	
	if (val == -1) perror("semdt ERROR ");					//in case of error
	return;
}



int main(int argc, char *argv[]) {
	int m=0,n=0;
	int shmid,shmidHead,shmidTail,shmidSum, status,id;
	int full,empty,mutex,val,*sum;
	struct sembuf pop, vop ;
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
	shmid = shmget(IPC_PRIVATE, BUFFER_SIZE*sizeof(int), 0777|IPC_CREAT);
	if (shmid==-1) perror("shmget ERROR ");								//in case of error
	shmidHead = shmget(IPC_PRIVATE, 1*sizeof(int), 0777|IPC_CREAT);
	if (shmidHead==-1) perror("shmget ERROR ");							//in case of error
	shmidTail = shmget(IPC_PRIVATE, 1*sizeof(int), 0777|IPC_CREAT);
	if (shmidTail==-1) perror("shmget ERROR ");							//in case of error
	shmidSum = shmget(IPC_PRIVATE, 1*sizeof(int), 0777|IPC_CREAT);
	if (shmidSum==-1) perror("shmget ERROR ");							//in case of error
	
	/* In the following  system calls, the second parameter indicates the
	   number of semaphores under this semid. Throughout this lab,
	   give this parameter as 1. If we require more semaphores, we
	   will take them under different semids through separate semget()
	   calls.
	*/

	//Creating three semaphores, one for mutex (to ensure mutual exclusion) so that only one process can modify (write into or modify head/tail pointers) the shared circular buffer
	//one for keeping a count of number of places in the buffer that are currently full
	//one for keeping a count of number of places in the buffer that are currently empty
	full = semget(IPC_PRIVATE, 1, 0777|IPC_CREAT);
	if (full == -1) perror("semget ERROR ");							//in case of error
	empty = semget(IPC_PRIVATE, 1, 0777|IPC_CREAT);
	if (empty == -1) perror("semget ERROR ");							//in case of error
	mutex = semget(IPC_PRIVATE, 1, 0777|IPC_CREAT);
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


	//Taking the number of producers (m) and number of consumers (n) from the user
	printf ("Enter m : ");
	scanf("%d",&m);

	printf ("Enter n : ");
	scanf("%d",&n);
	
	//creating new process
	id=fork();

	//child process used to create consumer processes
	if (id==0) {
		int desiredSum=m*25*51;										//sum to be achieved, used for exiting the infinite loop of consumer processes
		int j=0,idc;

		for(j=0;j<n;j++) {											//a for loop to create n consumer processes
			idc=fork();												//new process is created
			if(idc==0) {
				//Acessing the SUM shared memory
				sum = (int *) shmat(shmidSum, 0, 0);
				if (*sum==-1) perror("shmat ERROR ");				//in case of error
				
				while(1) {											//infinite loop to keep reading the non empty buffer until desired sum is achieved
					consumer(empty,full,mutex,shmid,shmidHead,shmidTail,shmidSum,pop,vop);		//calling the consumer process
					//printf("consumer %d : %d\n",j,sum[0]);
					//fflush(0);
					if (sum[0]==desiredSum) exit(0);				//if desired sum is achieved, exit
				}	
				//Detaching the SUM shared memory
				val=shmdt(sum);
				if (val == -1) perror("semdt ERROR ");				//in case of error
			}
			else if(idc==-1) {
				perror("fork ERROR ");								//in case of error
			}
			else continue;
		}
		wait(NULL);													//waiting for child process to finish
		exit(0);													//exiting the child process
	}	

	else if(id==-1) {
		perror("fork ERROR ");										//in case of error
	}

	//parent process for creating producer processes
	else{
		int j=0,idp,wpid;
		for(j=0;j<m;j++) {											//a for loop to create m producer processes
			idp=fork();												//new process is created
			if(idp==0) {
				//Acessing the SUM shared memory
				sum = (int *) shmat(shmidSum, 0, 0);
				if (*sum==-1) perror("shmat ERROR ");				//in case of error

				int i=0;
				for(i=0;i<NUMBER_OF_ITERATIONS;i++) {				//a producer process writes into the circular buffer 1 to 50 one by one
					producer(i+1,empty,full,mutex,shmid,shmidHead,shmidTail,shmidSum,pop,vop);	//calling the producer process, i+1 defines the number to be written into the next empty space in the circular buffer
					//printf("producer %d : %d\n",j,sum[0]);
					//fflush(0);
				}
				//Detaching the SUM shared memory
				val=shmdt(sum);
				if (val == -1) perror("semdt ERROR ");				//in case of error
				exit(0);
			}
			else if(idp==-1) {
				perror("fork ERROR ");								//in case of error
			}
			else continue;
		}
	}

	int wpid;
	while ((wpid = wait(&status)) > 0); 							//Waiting for all the child process to fininsh to ensure SUM is correctly updated
	//sleep(1);

	//Accessing the SUM shared memory
	sum = (int *) shmat(shmidSum, 0, 0);
	if (*sum==-1) perror("shmat ERROR ");							//in case of error

	printf("SUM : %d\n",sum[0]);									//Printing the final updated SUM shared memory value after all the producers and consumers processes have successfully exited
	fflush(0);
	
	//Detaching the SUM shared memory
	val=shmdt(sum);
	if (val == -1) perror("semdt ERROR ");							//in case of error

	val=semctl(full, 0, IPC_RMID, 0);
	if (val == -1) perror("semctl ERROR ");							//in case of error
	val=semctl(empty, 0, IPC_RMID, 0);
	if (val == -1) perror("semctl ERROR ");							//in case of error
	val=semctl(mutex, 0, IPC_RMID, 0);
	if (val == -1) perror("semctl ERROR ");							//in case of error

	val=shmctl(shmid, IPC_RMID, 0);
	if (val == -1) perror("semctl ERROR ");							//in case of error
	val=shmctl(shmidHead, IPC_RMID, 0);
	if (val == -1) perror("semctl ERROR ");							//in case of error
	val=shmctl(shmidTail, IPC_RMID, 0);
	if (val == -1) perror("semctl ERROR ");							//in case of error
	val=shmctl(shmidSum, IPC_RMID, 0);
	if (val == -1) perror("semctl ERROR ");							//in case of error

	return 0;
}