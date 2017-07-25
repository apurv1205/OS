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

//structure to store records in the shared memory
typedef struct t{
	char fname[20];
	char lname[20];
	int roll;
	float cg;
} data;

#define P(s) semop(s, &pop, 1)  /* pop is the structure we pass for doing
				   the P(s) operation */
#define V(s) semop(s, &vop, 1)  /* vop is the structure we pass for doing
				   the V(s) operation */

int shmidStatus;
void my_handler(int signum)//registering a signal handler to remove the shmidStatus shared memory in case ctrl+c is pressed
{
	shmctl(shmidStatus, IPC_RMID, 0);
	exit(-1);
}

int main(int argc,char* argv[]){

	//if the format for command line input is incorect, program terminates giving an error
	if(argc!=2) {
		printf("Incorrect format\nProvide the filename without spaces\nExample ./a.out filename\n");
		return 0;
	}

	int shmid,shmidn,semid1,semid2,shmid1,shmid2;
	struct sembuf pop,vop; 
	key_t key0,key1,key2,key3,key4,key5,key6; //keys are used to connect semaphores and shared memory across processes

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

	//this shared memory is to store data (records read from file)
    shmid = shmget(key1, 100*sizeof(data), 0777|IPC_CREAT);	/* We request an array of 100 data records (structure) */
    if(shmid <0) {
    	perror("ERROR ");
    	return 0;
    }

    //this shared memory is to tell Y processes tha X has been created, they can proceed now
    shmctl(shmidStatus, IPC_RMID, 0);//deleting already existing shared memory
	shmidStatus = shmget(key0, 1*sizeof(int), 0777|IPC_CREAT);
	 if(shmidStatus <0) {
    	perror("ERROR ");
    	return 0;
    }

    //this shared memory is for number of records in data record shared memory
	shmidn = shmget(key2, 1*sizeof(int), 0777|IPC_CREAT);
    if(shmidn <0) {
    	perror("ERROR ");
    	return 0;
    }

    //this is the writer semaphore in the reader writer problem
	semid1 = semget(key3, 1, 0777|IPC_CREAT);
	if(semid1 <0) {
    	perror("ERROR ");
    	return 0;
    }
	
	//this is the mutex semaphore in the reader writer problem
	semid2 = semget(key4, 1, 0777|IPC_CREAT);
	if(semid2 <0) {
    	perror("ERROR ");
    	return 0;
    }

    //this is the readcount shared memory for Y processes to be used in reader writer problem
   	shmid1 = shmget(key5, 1*sizeof(int), 0777|IPC_CREAT);
    if(shmid1 <0) {
    	perror("ERROR ");
    	return 0;
    }

    //this is the modified shared memory to tell X process that something has been updated
    shmid2 = shmget(key6, 1*sizeof(int), 0777|IPC_CREAT);
    if(shmid2 <0) {
    	perror("ERROR ");
    	return 0;
    }

    int val;
    val=semctl(semid1, 0, SETVAL, 1);	//initialising writer semaphore to 1
	if (val == -1) perror("semctl ERROR ");								//in case of error
	val=semctl(semid2, 0, SETVAL, 1);	//initialising mutex semaphore to 1
	if (val == -1) perror("semctl ERROR ");								//in case of error
	int *readcount;
	readcount=(int *)shmat(shmid1,0,0);	//attatching readcount shared memory and initialising to 0
	readcount[0]=0;
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

    char c[1000];
    FILE *fptr;
	char * line = NULL;
    size_t len = 0;
    ssize_t read;
	char row[1000]="";
	data *record;
    record = (data *)shmat(shmid, 0, 0);		//shared memory for student records
	int* n;
	n=(int *)shmat(shmidn,0,0);					//for number of records currently
    if ((fptr = fopen(argv[1], "r")) == NULL)	//in case of read error
    {
        perror("ERROR ");
		return 0;     
    }

    int i=0;
    while ((read = getline(&line, &len, fptr)) != -1) {//reading line by line
        strncpy(row, line, strlen(line)-1);
        int j=0;
		char *token = strtok(row," \t");	//delimiting each line by whitespaces

		while (token) {
			if (j==0) {		//if first name 
				int k=0;
				for (k=0;k<strlen(token);k++) record[i].fname[k]=token[k];
				record[i].fname[k]='\0';
			} 
			else if (j==1) {//if last name 
				int k=0;
				for (k=0;k<strlen(token);k++) record[i].lname[k]=token[k];
				record[i].lname[k]='\0';
			} 
			else if (j==2) {//if roll
				int tmp=0,val;
				val=sscanf(token, "%d", &tmp);//string to int conversion
				if(val==-1) {
					perror("ERROR ");
					return 0;
				}
				int k=0;
				for(k=0;k<i;k++) {
					if(record[k].roll==tmp) {
						printf("ERROR : Same roll repeated twice\n");
						return 0;
					}
				}
				record[i].roll=tmp;
			}
			else if (j==3) {//if cg
				float tmp1=0.0;
				int val1;
				val1=sscanf(token, "%f", &tmp1);//string to float conversion
				if(val1==-1) {
					perror("ERROR ");
					return 0;
				}
				record[i].cg=tmp1;
			}
			else {//if more than 4 space sperated words in a line
				printf("Invalid input data format in file, cant have more than 4 fields for a student\n");
				return 0;
			}
			token = strtok(NULL, " \t");//delimiting by whitespace
			j++;//incrementing word counter
		}
		if(j!=4) {//if any line has less than 4 fields
			printf("Invalid input data format in file,cant have less than 4 fields for a student\n");
			return 0;
		}

		i++;//incrementing line counter
        errno=0;
    }
    n[0]=i;//number of records
    if (read==-1) {
    	if(errno==0) printf("\nFile read successful ! Total records read = %d\n",n[0]);
    	else perror("ERROR ");
    }

    //freeing allocated variables and closing files
	free(line);
    fclose(fptr);

/*
    printf("The records read are as follows :\n\n");
    i=0;
    for(i=0;i<n[0];i++) {
    	printf("%s|%s|%d|%f\n",record[i].fname,record[i].lname,record[i].roll,record[i].cg);
    }
*/
    int *status,*modified;
    status=(int *)shmat(shmidStatus,0,0); //for telling Y that X has been created
    modified=(int *)shmat(shmid2,0,0);	//for telling X that data has been modified
    status[0]=1;

    int k=0;
    modified[0]=0;

    // register signal handler fo Ctrl-C
	signal(SIGINT, my_handler);

    while(1) {
    	status[0]=1;		//for telling Y that X has been created
    	sleep(5);
    	if (modified[0]==1) {//if shared data record has been modified
    		printf("Modified!\n");
    		if ((fptr = fopen(argv[1], "w")) == NULL)//in case of error
			{
			    perror("ERROR ");
				return 0;     
			}
    		for(i=0;i<n[0];i++) {//printing in the file in case of modification
    			fprintf(fptr,"%s %s %d %f\n",record[i].fname,record[i].lname,record[i].roll,record[i].cg);
    		}
    		fclose(fptr);
    		modified[0]=0;	//reseting modified shared data
    	}
    	else printf("No change!\n"); 	//no modification after 5 seconds
    	k++;							//number of iterations counter
    }


	return 0;
}