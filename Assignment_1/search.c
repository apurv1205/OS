#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//search function which recursively searches the entire array by dividing into two child process and then merging them
void search(int array[],int last,int first,int k){
	//if array to be searched is less than or equal to 10, we do linear search
	if(last-first+1<=10){
		int i=0;
		for(i=first;i<=last;i++){
			//if number is found, child process exits with status 1 (256)
			if (array[i]==k) exit(1);
		}
		//if number not found, the child process  exits with status 0 (0)
		exit(0);
	}

	//if array to be searched is greater than to 10, we form two child process and provide them half of array to search and finally
	//wait for their completion and then exit with suitable status accordingly
	else {
		int mid=(last+first)/2,id1=0,id2=0,status1=0,status2=0;
		//forming first child process
		id1=fork();
		//if first child process
		if(id1==0) {
			//using the search function to recursively search the array by divide and conquer using processes 
			search(array,mid,first,k);
		}
		//forming second child process with the same parent
		id2=fork();
		//if second child process
		if(id2==0) {
			//searching the other half of array recursively
			search(array,last,mid+1,k);
		}

		//parent process waiting for the first child process to finish
		waitpid(id1, &status1, 0);
		//parent process waiting for the second child process to finish
		waitpid(id2, &status2, 0);

		//if either process returns 1 (256) the parent process exits with status 1
		//else parent process exits with status 0
		if (status1+status2 > 0) exit(1);
		else exit(0);
	}
}



int main(){
	char f[100],num[100];
	int *array,k=-1,count=0,i=0;

	//Storing the filename in f (max 100 length)
	printf("\nEnter the name of the file : \n");
	scanf("%s",f);
	FILE * file;
	//reading in the file f
	file = fopen( f, "r");
	if (file) {
		//Reading till End Of File
	    while (fscanf(file, "%s", num)!=EOF) {
	    	//for keeping a count of the number of entries in the file
	    	count++;
	    }
	    fclose(file);
	}

	//allocating memory to array according to the number of entries in the file
	array=(int *)malloc(sizeof(int)*count);

	//reading in the file f
	file = fopen( f, "r");
	if (file) {
		//Reading till End Of File
	    while (fscanf(file, "%s", num)!=EOF) {
	    	//storing the string read into array[i] of type int
	    	sscanf(num, "%d", &array[i]);
	    	i++;
	    }
	    fclose(file);
	}

	//taking the number to be searched from the user (k >= 0)
	printf("Enter the positive number to search\n");
	scanf("%d",&k);

	//if k < 0  terminate with error message
	if(k<0) {
		printf("Entered number is not positive\nProgram terminated\n");
		return 0;
	}

	//if the array length is less than or equal to 10, do linear search
	if(count<=10) {
		int i=0;
		for(i=0;i<=count-1;i++){
			if (array[i]==k) {
				printf("\nNumber Found !\n");
				return 0;
			}
		}
		printf("\nNumber Not Found !\n");
	}

	//else we form 2 child processes and call the search function which recursively searches the array using same mechanism
	else{
		int status1=0,status2=0,mid=(count-1)/2;

		//forming the first child and calling the search function with 1st half of the array
		int id1=fork();
		if(id1==0) {
			search(array,mid,0,k);
		}

		//forming the second child and calling the search function with 2nd half of the array
		int id2=fork();
		if(id2==0) {
			search(array,count-1,mid+1,k);
		}
		//parent process waiting for the first child process to finish
		waitpid(id1, &status1, 0);
		//parent process waiting for the second child process to finish
		waitpid(id2, &status2, 0);

		//if either process returns 1 (256) it means that number has been found somewhere
		//else number not found in the file
		if (status1+status2 > 0) printf("\nNumber Found !\n");
		else printf("\nNumber Not Found !\n");
	}
	return 0;
}