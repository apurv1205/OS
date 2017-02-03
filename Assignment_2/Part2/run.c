#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char *argv[])
{
	//if the format for command line input is incorect, program terminates
	if(argc!=1) {
		printf("Incorrect format\nNo arguements required\n");
		return 0;
	}
	
	int id, status = 0;
	
	//forming a new process
	id = fork();

	//child process, our shell program runs in this process is done in this process
	if (id == 0) {
		//executable name in shell so compile shell.c as "gcc shell.c -o shell"
		
		//forming a new process to compile the shell.c program in a child process
 		int id_new = fork();

		if(id_new==0){//child process
			char *str1[5];
			str1[0]="gcc";
			str1[1]="shell.c";
			str1[2]="-o";
			str1[3]="./shell";
			str1[4]=NULL;

			//compiling shell.c with shell as executable after which this process finishes
			execv("/usr/bin/gcc", str1);
			//some error encountered in execv as it didn't run
			perror("execv ERROR ");
	       	exit(-1);
		}
		else if (id_new==-1) {//in case of fork error
			printf("fork ERROR ");
			exit(-1);
		}
		else {//parent process
			//waiting for child process to exit
			wait(&status);

			//executing our shell program after which the child process is finished
			char *str[2];
			str[1]="./shell";
			str[2]=NULL;
			execl("/usr/bin/xterm", "xterm","-e","./shell",str,NULL);
			//execvp("./shell",str);
	      	//some error encountered in execvp as it didn't run
			perror("execvp ERROR ");
	       	exit(-1);
       }
	
	}
	//Parent waits for the child process to finish and then exits itself
	wait(&status);
	printf("++++++Exiting the run program and the parent process\n");
	exit(-1);
	return 0;
}
