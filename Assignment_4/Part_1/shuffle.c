#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

//structure to store our id of each threads, it is passed as the arguement to the thread_start function
typedef struct t {
	int t_index;
	int start;
	int end;
} thread_data;

//global_count is for the number of threads that have finished row shifts and are now waiting for the cond signal to proceed further
//global_count1 is for the number of threads that have finished col shifts and are now waiting for the cond1 signal to proceed further
//k_count is the number of shuffles (row and col shift) done till now, end loop when k_count=k
int global_count=0,global_count1=0,k_count=0;
//k and x are for the mentioned variables in the question and n is for the number of rows/columns input from the user
int k,x,n;
//To store the n*n matrix
int **a;
pthread_t *thread_id=NULL;	//for pthread_t array of size x
pthread_mutex_t cnt_mutex;	//one mutex
pthread_cond_t cnt_cond;	//one cond for checking if all row shifts done or not
pthread_cond_t cnt_cond1;	//one cond for checking if all col shifts done or not

//Here we are using one mutex two global_count : 
//	global_count : for keeping a count of threads done with row shifts
// 	global_count1: fot keeping a count of threads done with column shifts
//and two condition variables one for checking if all row shifts done and one for checking if all column shifts done
//after row shifts, we lock the mutex, increment row global_count and if it is the last thread (global_count==x),
//we reset global_count to 0 and broadcast the row condition variable signalling end of all row shifts
//then we have while (...) wait for row signal and then finally unlock mutex and then the column shifts as all row shifts are finished now
//then it is just like row shift synchronisation but we use different global_count and condition variable to seperate the row shifts and columns shifts
void *thread_start(void *param)		//this fuction is called with each new thread created
{
	int val;
	thread_data *t_param = (thread_data *) param;
	int tid = (*t_param).t_index;	//our assigned id to the thread between 0 and x-1
	while (k_count < k ){			//k_count is the number of shuffles done
		int start=(*t_param).start;
		int end=(*t_param).end;
		//row shift
		int i=0;
		for (i=start;i<=end;i++) {
			int j=0;
			int temp=a[i][0];
			for (j=0;j<n-1;j++) {
				a[i][j]=a[i][j+1];
			}
			a[i][n-1]=temp;
		}

		val=pthread_mutex_lock(&cnt_mutex);	//locking the mutex so only one active thread can enter this critical section at a time
		if (val!=0) {
			perror("mutex lock ERROR ");
			exit(0);
		}
		global_count++;						//incrementing row global count	


		if(global_count  == x ) {		//if it is the last thread
			val=pthread_cond_broadcast(&cnt_cond);		// signal to the waiting processes if the condition is successful.
			if (val!=0) {
				perror("cond broadcast ERROR ");
				exit(0);
			}
			global_count=0;				//reseting the row golbal count
		}

		// while is used instead of just if as Posix standard
		// allows spurious wakeups even if the condition is \
		// not satisfied.
		// By property of condition variables, cnt_mutex will be
		// automatically unlocked if the thread blocks
		while (global_count%x!=0) {
			val=pthread_cond_wait(&cnt_cond, &cnt_mutex);//if not the last thread, block the thread, unlock mutex till condition is met
			if (val!=0) {
				perror("cond wait ERROR ");
				exit(0);
			}
		}
		val=pthread_mutex_unlock(&cnt_mutex);	//after condition is met, it automatically locks the mutex, so needs to be unlocked
		if (val!=0) {
			perror("mutex unlock ERROR ");
			exit(0);
		}

		//column shift
		int j=0;
		for (j=start;j<=end;j++) {
			int i=0;
			int temp=a[n-1][j];
			for (i=n-1;i>0;i--) {
				a[i][j]=a[i-1][j];
			}
			a[0][j]=temp;
		}

		val=pthread_mutex_lock(&cnt_mutex);	//locking the mutex so only one active thread can enter this critical section at a time
		if (val!=0) {
			perror("mutex lock ERROR ");
			exit(0);
		}	
		global_count1++;				//incrementing column global count	

		if(global_count1 == x ) {		//if it is the last thread
			val=pthread_cond_broadcast(&cnt_cond1);		// signal to the waiting processes if the condition is successful.
			if (val!=0) {
				perror("cond broadcast ERROR ");
				exit(0);
			}	
			global_count1=0;			//reseting the column golbal count
			k_count++;					//incrementing the no. of shuffles
		}	

		// while is used instead of just if as Posix standard
		// allows spurious wakeups even if the condition is \
		// not satisfied.
		// By property of condition variables, cnt_mutex will be
		// automatically unlocked if the thread blocks
		while (global_count1%x!=0) {
			val=pthread_cond_wait(&cnt_cond1, &cnt_mutex);//if not the last thread, block the thread, unlock mutex till condition is met
			if (val!=0) {
				perror("cond wait ERROR ");
				exit(0);
			}
		}
		val=pthread_mutex_unlock(&cnt_mutex);//after condition is met, it automatically locks the mutex, so needs to be unlocked
		if (val!=0) {
			perror("mutex unlock ERROR ");
			exit(0);
		}
	}
	pthread_exit(NULL); //exit the threads
}

int main(){
	scanf("%d",&n);		//taking no. of rows/columns from the user
	a=(int **)malloc(sizeof(int *)*n);	//allocating memory to out pointer
	int i=0;
	printf("Entered number of rows/columns for the matrix : %d\n",n);	//printing entered no. of rows/columns
	for (i=0;i<n;i++) {
		a[i]=(int *)malloc(sizeof(int)*n);	//allocating memory to out pointer
		int j=0;
		for(j=0;j<n;j++) {
			scanf("%d",&a[i][j]);			//storing read integer in out 2*2 array
		}
	}

	//printing the read matrix
	printf("The read matrix is as follows :\n\n");
	i=0;
	for (i=0;i<n;i++) {
		int j=0;
		for(j=0;j<n;j++) {
			printf("%d\t",a[i][j]);
		}
		printf("\n\n");
	}
	x=0;
	k=0;
	scanf("%d",&k);			//reading k from user
	scanf("%d",&x);			//reading x from user
	if(x > n) {				//incase x is greater than n
		printf("ERROR : Entered value of x is greater than n\n");
		return 0;
	}
	printf("Entered k and x respectively are : %d %d\n",k,x); 	//printing the read k and x

	thread_id=(pthread_t *)malloc(sizeof(pthread_t)*x);			//allocating memory to our pthread_t array pointer
	thread_data *param;
	param=(thread_data *)malloc(sizeof(thread_data)*x);			//allocating memory to our thread_data structure pointer
	
	// initialize the mutex and the condition variable
	int val=0;
	val=pthread_mutex_init(&cnt_mutex, NULL);
	if (val!=0) {
		perror("mutex init ERROR ");
		exit(0);
	}
	val=pthread_cond_init(&cnt_cond, NULL);		//for checking row shifts
	if (val!=0) {
		perror("cond init ERROR ");
		exit(0);
	}
	val=pthread_cond_init(&cnt_cond1, NULL);	//for checkin column shifts
	if (val!=0) {
		perror("cond init ERROR ");
		exit(0);
	}
	
	// create (x) threads
	for(i=0; i<x; i++)
	{
		int start=0,end=0;			//for calculating the start and end indices of the row/col to shift
		start=i*(n/x);		
		if (i== (x-1)) {			//if last thread, it's end index is n-1 as it's end - start can be more than n/x
			end=n-1;
		}
		else {						//if not last thread, its end is the start of next thread - 1
			end=(i+1)*(n/x) - 1;
		}
		param[i].t_index = i;		//assigning our custom id to the param[i].t_index
		param[i].start = start;
		param[i].end = end;
		val=pthread_create(&thread_id[i], NULL, thread_start, (void *) &param[i]);	//creating thread, the thread calls the thread_start function with param[i] as arguement
		if (val!=0) {
			perror("pthread create ERROR ");
			exit(0);
		}
	}

	// wait for all threads to finish
	for(i=0; i<x; i++)
	{
		val=pthread_join(thread_id[i], NULL);
		if (val!=0) {
			perror("pthread join ERROR ");
			exit(0);
		}
	}

	//All threads terminated, now print the final matrix
	printf("All threads terminated\nThe final matrix after %d shuffles is as follows :\n\n",k);
	i=0;
	for (i=0;i<n;i++) {
		int j=0;
		for(j=0;j<n;j++) {
			printf("%d\t",a[i][j]);
		}
		printf("\n\n");
	}

	// clean up the mutex and condition variable
	val=pthread_mutex_destroy(&cnt_mutex);
	if (val!=0) {
		perror("mutex destroy ERROR ");
		exit(0);
	}
	val=pthread_cond_destroy(&cnt_cond);
	if (val!=0) {
		perror("cond destroy ERROR ");
		exit(0);
	}
	val=pthread_cond_destroy(&cnt_cond1);
	if (val!=0) {
		perror("cond destroy ERROR ");
		exit(0);
	}

	return 0;	
}