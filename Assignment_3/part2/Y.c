#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include  <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <sys/sem.h>

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


int main(int argc,char* argv[]){

	key_t key0,key1,key2,key3,key4,key5,key6; //keys are used to connect semaphores and shared memory across processes
	struct sembuf pop,vop;
	int *status;
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
	if(key2==-1) {
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

	int shmid,shmidStatus,shmidn,semid1,semid2,shmid1,shmid2;
	//this is for checking if X has been created and shared data initialised	
	shmidStatus = shmget(key0, 1*sizeof(int), 0777|IPC_CREAT);
	if (shmidStatus<0) {
		perror("ERROR ");
    	return 0;
	}
	status=(int *)shmat(shmidStatus,0,0); //if X is not created status[0]=0
	if (status[0]!=1) {printf("Process Y created before process X, waiting for X to start\n");
		while(status[0]!=1);//waiting for X to start
	}

	//this shared memory is to store data (records read from file)
	shmid = shmget(key1, 100*sizeof(data), 0777|IPC_CREAT);	/* We request an array of 100 data records (structure) */
    if(shmid <0) {
    	perror("ERROR ");
    	return 0;
    }
    
    //this shared memory is for number of records in data record shared memory
	shmidn = shmget(key2, 1*sizeof(int), 0777|IPC_CREAT);	/* We request an array of 100 data records (structure) */
    if(shmid <0) {
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
   	shmid1 = shmget(key5, 1*sizeof(int), 0777|IPC_CREAT);	/* We request an array of 100 data records (structure) */
    if(shmid1 <0) {
    	perror("ERROR ");
    	return 0;
    }

    //this is the modified shared memory to tell X process that something has been updated    
    shmid2 = shmget(key6, 1*sizeof(int), 0777|IPC_CREAT);	/* We request an array of 100 data records (structure) */
    if(shmid2 <0) {
    	perror("ERROR ");
    	return 0;
    }
    int *n,*readcount,*modified;
    n=(int *)shmat(shmidn,0,0);				//for number of records currently
    modified=(int *)shmat(shmid2,0,0);		//for telling X that data has been modified
    readcount=(int *)shmat(shmid1,0,0);		//readcount shared data for reader writer problem
    data *record;
    record = (data *) shmat(shmid, 0, 0);	//shared memory for student records
	int input = 0;

	while(input!=3) {//till exit is selected
		printf("\nShared memory loaded successfully, enter the querries as follows :\n");//prompt for input
		printf("   1 : Search for a student record by roll no.\n   2 : Update the CGPA of a particular student ( by roll no. )\n   3 : Exit\n\n");
		scanf("%d",&input);

		if(input==1) {//search selected
			int roll=0,i=0,flag=0;
			printf("Enter the roll number to search : ");
			scanf("%d",&roll);//reading roll to search
			for(i=0;i<n[0];i++) {//searching in the shared data for the roll mentioned
				if(record[i].roll==roll) {//if roll found
					flag++;
					//reader process
					P(semid2);
					readcount[0]++;
					if (readcount[0]==1) P(semid1);
					V(semid2);
					printf("Record for %s %s found : ",record[i].fname,record[i].lname);
					printf("\n   | %s | %s | %d | %f |\n",record[i].fname,record[i].lname,record[i].roll,record[i].cg);
					P(semid2);
					readcount[0]--;
					if (readcount[0]==0) V(semid1);
					V(semid2);
				}
			}
			if (flag==0) printf("No record found for the entered roll number\n");

		}
		else if (input==2) {//update selected
			int roll=0,i=0,flag=0,j=-1;
			float cgNew=0.0;
			printf("Enter the roll number to update : ");
			scanf("%d",&roll);//reading roll to update
			for(i=0;i<n[0];i++) {//searching in the shared data for the roll mentioned
				if(record[i].roll==roll) {//if roll found
					flag++;
					j=i;
					printf("Record for %s %s found : ",record[i].fname,record[i].lname);
					printf("\n   | %s | %s | %d | %f |\n",record[i].fname,record[i].lname,record[i].roll,record[i].cg);
					printf("\nEnter the new CGPA : ");
					scanf("%f",&cgNew);//taking new cg from user
					break;
				}
			}
			if (flag==0) printf("No record found for the entered roll number\n");
			//writer process
			P(semid1);
			if (j!=-1) record[j].cg=cgNew;//updating cg here if record found
			V(semid1);
			modified[0]=1;
			printf("CGPA updated successfully !\n");
		}
		else if(input!=3) {printf("Incorrect querry\n");exit(-1);}//if anything other than 1,2 or 3 entered
	}
	printf("Exited the process Y\n"); //3 selected

	//deataching shared memory from local variables
	shmdt(modified);
  	shmdt(status);
    shmdt(record);//detaching the shared memory from record local variable
	shmdt(n);
	shmdt(readcount);
    //removing shared memory
    /*
    semctl(semid1, 0, IPC_RMID, 0);
	semctl(semid2, 0, IPC_RMID, 0);
	shmctl(shmid, IPC_RMID, 0);
	shmctl(shmidStatus, IPC_RMID, 0);
	shmctl(shmidn, IPC_RMID, 0);
	shmctl(shmid1, IPC_RMID, 0);
	shmctl(shmid2, IPC_RMID, 0);
	*/
	return 0;
}