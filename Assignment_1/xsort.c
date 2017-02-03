#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char *argv[])
{
	//if the format for command line input is incorect, program terminates
	if(argc!=2) {
		printf("Incorrect format\nProvide the filename( without spaces )....for example\n./xsort file.txt\n");
		return 0;
	}
	
	int id, status = 0;
	
	//forming a new process
	id = fork();

	//child process, sorting is done in this process
	if (id == 0) {

		//execl takes first arguement as the xterm environment path and second,third and fourth as the xterm command and its options.
		//fifth is the filename passed through command line to the xsort program. So it will open a new xterm window with the numbers in file displayed in the
		//accending sorted manner.
		execl("/usr/bin/xterm", "xterm", "-hold","-e","./sort1",argv[1],NULL);
      	//some error encountered in execvp as it didn't run
		perror("execl failed\n");
       	exit(-1);
	
	}
	//Parent waits for the child process to finish
	wait(&status);

	return 0;
}

