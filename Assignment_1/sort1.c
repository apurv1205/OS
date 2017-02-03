#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

int main(int argc, char *argv[])
{
	//array stores the numbers in the file in unsorted order
	//num stores the individual numbers in the file
	int array[1000]={0},i=0,count=0; 
	char num[100];

	//opening the file with filename as argv[1]
	FILE * file;
	file = fopen( argv[1], "r");
	if (file) {
		//while End Of File is not reached
	    while (fscanf(file, "%s", num)!=EOF) {
	    	//array[i] stores the current number in file
	    	sscanf(num, "%d", &array[i]);
	    	i++;
	    	count++;
	    }
	    fclose(file);
	}

	//sorting the array in ascending order using the most intuitive selection sort
	int temp=0,j=0,k=0,min=INT_MAX;
	for(i=0;i<count;i++){
		min=INT_MAX;
		for(j=i;j<count;j++) {
			if (array[j]<min) {
				k=j;
				min=array[j];
			}
		}
		//swapping the current min with array[i]
		temp=array[k];
		array[k]=array[i];
		array[i]=temp;
	}

	//displaying the output in the command line
	for (i=0;i<count;i++) printf("%d\n",array[i]);

	exit(1);

	return 0;
}