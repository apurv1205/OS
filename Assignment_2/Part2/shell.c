#include <limits.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <grp.h>
#include <pwd.h>


//recursive funciton to form child processes and link them to other processes to run more than 2 piped processes
void pipes(int i,int rd,int wr,char *args[100][100]) {
	if(i==0) {				//innermost executable, takes input from stdin and passes output
		close(rd);			//to wr that is the writing end of the parent process's pipe

		close(1);	 		/* Close the file descriptor for stdout */
		dup(wr);		 	/* Duplicate wr at the lowest-numbered unused descriptor */
		close(wr);			/* wr is no longer needed */
		
		//executing files
        execvp(args[i][0],args[i]);	//executing the leftmost executable, input : stdin, output : wr
        perror("execvp ERROR for executable ");	//in case of error
        return;
	}

	else{	//if left most executable is not reached,
		//a child process is formed and pipe called recursively where i, the executable count is decreased 
		int id,status,fd[2];
	    pipe(fd);//to communicate between the current and left to current executable

		//create a child process for left executable
		id=fork();
		//if child process
		if(id==0)
		{
			pipes(--i,fd[0],fd[1],args);		//recursive call to execute the left executables
		}

		//fork ERROR
		else if (id==-1) {
			printf("fork ERROR for child ");
			return;
		}
		
		//current executable process
		else
		{
			wait(&status); 		//if not a background program, parent waits for child to finish
			
			close(fd[1]);		//close the writing end of the pipe
			close(rd);			//close the reading end of parent's pipe

			close(0);	 		/* Close the file descriptor for stdin */
			dup(fd[0]);			/* Duplicate fd[0] at the lowest-numbered unused descriptor */
			close(fd[0]);		/* fd[0] is no longer needed */

			close(1);	 		/* Close the file descriptor for stdout */
			dup(wr);			/* Duplicate wr at the lowest-numbered unused descriptor */
			close(wr);			/* wr is no longer needed */
			
			//executing current executable after making sure it gets input from left executable and 
			//provides output to right executables
            execvp(args[i][0],args[i]);
            perror("execvp ERROR for second executable ");//executing the middle executable, input : fd[0], output : fd[1]
            return;
		}

	}

	return;
}


//main function of our shell
int main(int argc, char *argv[])
{
	//Welcome message for our terminal
	printf("***Welcome to the shell program !\n***version 1.0\n");
	
	while(1) {
		char pwd[1000]="",command[1000]="",input[1000]="",*ptr;

		//getting current working directory
		ptr = getcwd(pwd, sizeof(pwd));
		if(ptr==NULL) {//in case of error in getting current directory
			perror("getcwd ERROR ");
			continue;
		}

		//prompt for user to get command
		printf("%s> ", pwd);

		//getting command from user
		fgets (input, 1000, stdin);
		input[strlen(input)-1]='\0';

		//trimming leading whitespaces from the user input to get the command
		int flag=0;
		int i,j=0;
		for(i=0;input[i]!='\0';i++) {
			if (flag) command[j++]=input[i];
			if (input[i] != ' ' && input[i] != '\t' && flag==0) {
				command[j++]=input[i];
				flag=1;
			}
		}
		command[j]='\0';

/////////////Built-in Commands

		//if command is pwd
		if( strncmp(command,"pwd",3)==0 ) {
			printf ("+++The current working directory is : \"%s\"\n",pwd);
			continue;
		}

		//if command is cd
		else if(strncmp(command,"cd",2)==0 ){
			char dest[1000]="";

			//getting the destination path's first substring with no whitespaces in it
			strncpy(dest, command+3, strlen(command)-2 );
			char *no_space_dest = strtok (dest," \t");//delimiting dest by tabs or spaces

			//no_space_dest now stores the destination true path with no whitespace in it
			printf("+++Changing current directory to : \"%s\"\n",(no_space_dest));
       		int ret = chdir ((no_space_dest));
	        if(ret!=0){
	        	perror("cd ERROR ");
	        	printf("Failed to change current directory to \"%s\"\n",no_space_dest);
	        	continue;
	        }
			continue;
		}

		//if command is mkdir
		else if( strncmp(command,"mkdir",5)==0 ) {
			char dest[1000]="";

			//getting the destination path's first substring with no whitespaces in it
			strncpy(dest, command+6, strlen(command)-5 );
			char *no_space_dest = strtok (dest," \t");//delimiting dest by tabs or spaces

			//no_space_dest now stores the destination true path with no whitespace in it
			printf ("+++Creating new directory : \"%s\"\n",(no_space_dest));
			int ret=mkdir(no_space_dest,S_IRWXU);
			if(ret==-1){
				perror("mkdir ERROR ");
				printf("Failed to create new directory \"%s\"\n",no_space_dest);
				continue;
			}
			continue;
		}

		//if command is rmdir
		else if( strncmp(command,"rmdir",5)==0 ) {
			char dest[1000]="";

			//getting the destination path's first substring with no whitespaces in it
			strncpy(dest, command+6, strlen(command)-5 );
			char *no_space_dest = strtok (dest," \t");//delimiting dest by tabs or spaces

			//no_space_dest now stores the destination true path with no whitespace in it
			printf ("+++Removing directory : \"%s\"\n",(no_space_dest));
			int ret=rmdir(no_space_dest);
			if(ret==-1){
				perror("rmdir ERROR ");
				printf("Failed to remove directory \"%s\"\n",no_space_dest);
				continue;
			}
			continue;
		}

		//if command is ls
		else if( strncmp(command,"ls",2)==0 ) {
			DIR *curDir=NULL;
			struct dirent *readDir=NULL;

			//getting option for ls
			char option[1000]="";
			strncpy(option, command+3, strlen(command)-2 );

			//command is ls -l
			if(strncmp(option,"-l",2)==0) {
				printf ("+++Listing contents of directory \"%s\" in long listing format\n",pwd);
				curDir=opendir(pwd);
				if(curDir==NULL){//error in opening the current directory
					perror("opendir ERROR ");
					printf("Cannot open the directory : %s\n",pwd);
					continue;
				}

				while((readDir=readdir(curDir))!=NULL){//reading the contents of directory
					struct stat result;
					int ret=stat(readDir->d_name, &result);
					if(ret!=0) {
						perror("stat ERROR ");
						printf("Cannot display contents of directory \"%s\" in long listing format\n",pwd);
						continue;
					}

					printf( (S_ISDIR(result.st_mode)) ? "d" : "-");
					printf( (result.st_mode & S_IRUSR) ? "r" : "-");
					printf( (result.st_mode & S_IWUSR) ? "w" : "-");
					printf( (result.st_mode & S_IXUSR) ? "x" : "-");
					printf( (result.st_mode & S_IRGRP) ? "r" : "-");
					printf( (result.st_mode & S_IWGRP) ? "w" : "-");
					printf( (result.st_mode & S_IXGRP) ? "x" : "-");
					printf( (result.st_mode & S_IROTH) ? "r" : "-");
					printf( (result.st_mode & S_IWOTH) ? "w" : "-");
					printf( (result.st_mode & S_IXOTH) ? "x" : "-");

					//last access time of file
					char *tm=ctime(&result.st_atime);
					char tmStamp[100],uid[1000],gid[1000];
					strncpy(tmStamp, tm+4, strlen(tm)-5 );
					tmStamp[strlen(tmStamp)+1]='\0';
					//getting user name and group name
					struct passwd *pwd = getpwuid(result.st_uid);
					struct group *src_gr = getgrgid(result.st_gid);
					//printing in the desired format
					printf("  %d\t%s  %s\t%d\t%s\t%s\n",(int)result.st_nlink,pwd->pw_name,src_gr->gr_name,(int)result.st_size,tmStamp,readDir->d_name);
		
				}	
				
				int ret1=closedir(curDir);
				if(ret1==-1){//error in opening the current directory
					perror("closedir ERROR ");
					continue;
				}			
			}

			//command is just ls
			else if(strlen(command)==2){
				printf ("+++Listing contents of directory \"%s\"\n",pwd);
				curDir=opendir(pwd);
				if(curDir==NULL){//error in opening the current directory
					perror("opendir ERROR ");
					printf("Cannot open the directory : \"%s\"\n",pwd);
					continue;
				}
				
				int count=0;
				while((readDir=readdir(curDir))!=NULL){//reading the contents of directory
					count++;
					printf("%-30s", readDir->d_name);
					if(count==4) {
						count=0;
						printf("\n");
					}
				}
				if (count!=0) printf("\n");
				
				int ret1=closedir(curDir);
				if(ret1==-1){//error in opening the current directory
					perror("closedir ERROR ");
					continue;
				}
			}
			continue;
		}

		else if(strncmp(command,"cp",2)==0) {
			char subStr[1000]="";

			//getting the source file's path and destination's path with whitespace between them
			strncpy(subStr, command+3, strlen(command)-2 );
			char *source = strtok (subStr," \t");//delimiting source by tabs or spaces to get source file path
			char *dest = strtok (NULL," \t");//getting next substring delimited by tabs or spaces for dest file path

			struct stat result1,result2;
			int ret1=stat(source, &result1);
			if(ret1!=0) {
				perror("stat ERROR ");
				printf("Cannot get modificaion time of source file \"%s\"\n",source);
				continue;
			}
			int ret2=stat(dest, &result2);//for last modified time
			if(ret2!=0) {
				perror("stat ERROR ");
				printf("Cannot get modificaion time of destination file \"%s\"\n",dest);
				continue;
			}

			if (result1.st_mtime > result2.st_mtime) {
				printf("Copying from file \"%s\" to file \"%s\"\n",source,dest);
				int fp1, fp2;

				fp1=open(source, O_RDONLY );//read only
				fp2=open(dest, O_WRONLY );//write only
				char buff[200];

				if(fp1==-1) {//open error
					perror("fopen ERROR ");
					printf("Unable to open source file \"%s\"\n",source);
					continue;
				}
				if(fp2==-1) {//open error
					perror("fopen ERROR ");
					printf("Unable to open destinaito file \"%s\"\n",dest);
					continue;
				}
				while (1) {
					//reading 100 bytes from file1
					int rd=read(fp1,buff,100);
					//writing the read number of bytes to file2			
					int wr=write(fp2,buff,rd);

					//file read error
					if(rd==-1) {
						perror("read ERROR ");
						continue;
					}
					
					//file write error
					if(wr==-1) {
						perror("write ERROR ");
						continue;
					}
					
					//exit conditions : when read number of bytes is less than 100
					if( wr>100&&wr>=0 || rd==0 ){
						close(fp1);
						close(fp2);
						printf("File copied Successfully\n");
						break;
					}

				}
			}

			else printf("copy ERROR : The source file was found to be older than the destination file\n");
			continue;
		}

		//else if command is exit
		else if( strncmp(command,"exit",4)==0 ) {
			printf ("+++Exiting the shell program\n");
			exit(-1);
		}


		//else command is executable
		else {

			//seperating the input string into argv array by whitespaces
			char *arg[50],*arg1[50],*arg2[50],*arg3[50];
			char infile[100]="",outfile[100]="",pipe1[1000]="",pipe2[1000]="",pipe3[1000]="";

			int npipes=0,j=0;
			int backGround=0;

			for(j=0;j<strlen(command);j++){//cpunting the number of pipes in command 
				if(command[j]=='|') npipes++;
			}

			//if only one level of pipe
			if (npipes==1) {//parsing the command

				char *args[100][100];//for storing executables
				int j=0;
				i=0;
				char *token = strtok(command,"|");
				int i=0,in=0,out=0,ini=100,outi=100,pipeno=0;
				//dividing the string based on "|"
			    while (token) {
			    	args[i++][0]=token;
			        token = strtok(NULL, "|");//delimiting by whitespaces
			    }
			    //dividing the already divided string based on whitespaces for command line arguements of executables
			    for(j=0;j<npipes+1;j++){
				    char *token1 = strtok(args[j][0]," \t");
				    i=0;
				    while (token1) {
				    	args[j][i++]=token1;
				        token1 = strtok(NULL, " \t");//delimiting by whitespaces
				    }
				    args[j][i]=NULL;//last arguement for exec's second arguement is terminated by NULL
				}

			    int id1,status,fd[2];
			    pipe(fd);//to communicate between first and second executable

				//create a child process for second executable
				id1=fork();
				//if child process
				if(id1==0)
				{
					int id2;
					id2=fork();

					//if child 2 process
					if(id2==0)
					{
						close(fd[0]);

						close(1);	 		/* Close the file descriptor for stdout */
						dup(fd[1]);		 	/* Duplicate fd[1] at the lowest-numbered unused descriptor */
						close(fd[1]);		/* fd[1] is no longer needed */
						
						//executing files
			            execvp(args[0][0],args[0]); //executing the leftmost executable, input : stdin, output : fd[1]
			            perror("execvp ERROR for first executable ");
			            continue;
					}
					//fork ERROR
					else if (id2==-1) {
						printf("fork ERROR for second child ");
						continue;
					}
					//child 2 process
					else
					{
						wait(&status); //if not a background program, parent waits for child to finish
						close(fd[1]);

						close(0);	 		/* Close the file descriptor for stdin */
						dup(fd[0]);		 	/* Duplicate fd[0] at the lowest-numbered unused descriptor */
						close(fd[0]);		/* fd[0] is no longer needed */
						
						//executing files
			            execvp(args[1][0],args[1]); //executing the rightmost executable, input : fd[0], output : stdout
			            perror("execvp ERROR for second executable ");
			            continue;
					}

				}
				
				//fork ERROR
				else if (id1==-1) {
					printf("fork ERROR for first child ");
					continue;
				}
				
				//parent process
				else
				{
					wait(&status); //if not a background program, parent waits for child to finish
					printf("\n+++Executed command with one level of pipe Successfully\n");
				}

			}

			//if two levels of pipe
			else if(npipes==2) {//parsing the command
				
				char *args[100][100];//for storing executables
				int j=0;
				i=0;
				char *token = strtok(command,"|");
				int i=0,in=0,out=0,ini=100,outi=100,pipeno=0;
				//dividing the string based on "|"
			    while (token) {
			    	args[i++][0]=token;
			        token = strtok(NULL, "|");//delimiting by "|"
			    }
			    //dividing the already divided string based on whitespaces for command line arguements of executables
			    for(j=0;j<npipes+1;j++){
				    char *token1 = strtok(args[j][0]," \t");
				    i=0;
				    while (token1) {
				    	args[j][i++]=token1;
				        token1 = strtok(NULL, " \t");//delimiting by whitespaces
				    }
				    args[j][i]=NULL;//last arguement for exec's second arguement is terminated by NULL
				}

			    int id1,status,fd[2],fd1[2];
			    pipe(fd);//to communicate between first and second executable
				//create a child process for second executable
				id1=fork();
				//if child 1 process
				if(id1==0)
				{

					int id2;
					id2=fork();

					//if child 2 process
					if(id2==0)
					{
						pipe(fd1);//to communicate between second and third executable

						int id3;
						id3=fork();

						//if child 3 process
						if(id3==0)
						{

							close(fd1[0]);

							close(1);	 		/* Close the file descriptor for stdout */
							dup(fd1[1]);		/* Duplicate fd[1] at the lowest-numbered unused descriptor */
							close(fd1[1]);		/* fd[1] is no longer needed */
							
							//executing files
				            execvp(args[0][0],args[0]);	//executing the leftmost executable, input : stdin, output : fd1[1]
				            perror("execvp ERROR for first executable ");
				            continue;
						}
						//fork ERROR
						else if (id3==-1) {
							printf("fork ERROR for third child ");
							continue;
						}
						//child 2 process
						else
						{
							wait(&status); 		//if not a background program, parent waits for child to finish
			

							close(fd1[1]);
							close(fd[0]);

							close(0);	 		/* Close the file descriptor for stdin */
							dup(fd1[0]);		/* Duplicate fd1[0] at the lowest-numbered unused descriptor */
							close(fd1[0]);		/* fd1[0] is no longer needed */

							close(1);	 		/* Close the file descriptor for stdout */
							dup(fd[1]);			/* Duplicate fd[1] at the lowest-numbered unused descriptor */
							close(fd[1]);		/* fd[1] is no longer needed */
							
							//executing files
				            execvp(args[1][0],args[1]);
				            perror("execvp ERROR for second executable ");//executing the middle executable, input : fd1[0], output : fd[1]
				            continue;
						}

					}
					//fork ERROR
					else if (id2==-1) {
						printf("fork ERROR for second child ");
						continue;
					}
					
					//child 1 process
					else
					{
						wait(&status);		//if not a background program, parent waits for child to finish
						close(fd[1]);

						close(0);	 		/* Close the file descriptor for stdin */
						dup(fd[0]);		 	/* Duplicate fd[0] at the lowest-numbered unused descriptor */
						close(fd[0]);		/* fd[0] is no longer needed */
						
						//executing files
			            execvp(args[2][0],args[2]);	//executing the rightmost executable, input : fd[0], output : stdout
			            perror("execvp ERROR for third executable ");
			            continue;
					}
				}

				//fork ERROR
				else if (id1==-1) {
					printf("fork ERROR for first child ");
					continue;
				}
				
				//parent process
				else
				{
					wait(&status); //if not a background program, parent waits for child to finish
					printf("\n+++Executed command with two levels of pipe Successfully\n");
				}
			}

			//when number of pipes is more than 2, recursive function pipe is used to execute piped processes
			else if(npipes>2) {
				char *args[100][100];
				int j=0;
				i=0;
				char *token = strtok(command,"|");
				int i=0,in=0,out=0,ini=100,outi=100,pipeno=0;
			    while (token) {
			    	args[i++][0]=token;
			        token = strtok(NULL, "|");//delimiting by "|"
			    }

			    for(j=0;j<npipes+1;j++){
				    char *token1 = strtok(args[j][0]," \t");
				    i=0;
				    while (token1) {
				    	args[j][i++]=token1;
				        token1 = strtok(NULL, " \t");//delimiting by whitespaces
				    }
				    args[j][i]=NULL;
				}

				i=npipes;
				int id,status,fd[2];
			    pipe(fd);//to communicate between first and second executable

				//create a child process for second executable
				id=fork();
				//if child process
				if(id==0)
				{
					int id1=fork();
					if(id1==0) {
						pipes(--i,fd[0],fd[1],args);//child process calls the recursive pipe function to execute piped processes
						
					}

					//fork ERROR
					else if (id1==-1) {
						printf("fork ERROR for child ");
						continue;
					}
					
					//child 1 process
					else
					{
						wait(&status);		//if not a background program, parent waits for child to finish
						close(fd[1]);

						close(0);	 		/* Close the file descriptor for stdin */
						dup(fd[0]);		 	/* Duplicate fd[0] at the lowest-numbered unused descriptor */
						close(fd[0]);		/* fd[0] is no longer needed */
						
						//executing files
			            execvp(args[i][0],args[i]);	//executing the rightmost executable, input : fd[0], output : stdout
			            perror("execvp ERROR for executable ");
			            continue;
					}
				}
				//fork ERROR
				else if (id==-1) {
					printf("fork ERROR for child ");
					continue;
				}
				
				//child 1 process
				else
				{
					wait(&status);		//if not a background program, parent waits for child to finish
					printf("Returned from all %d piped processes\n",npipes+1);//all processes terminated successfully
		            continue;
				}
			}


			else{
				//parsing the input non pipe command
				char *token = strtok(command," \t");
				int i=0,in=0,out=0,ini=100,outi=100,pipeno=0;
			    while (token) {
			    	if(strncmp(token,"<",1)==0) {//incase of "<" in command
			    		in++;
			    	}
			    	else if(strncmp(token,">",1)==0) {//incase of ">" in command
			    		out++;
			    	}
			    	arg[i++]=token;
			        token = strtok(NULL, " \t");//delimiting by whitespaces
			        
			        if(in==1) {
			        	ini=i-1;//storing the index of "<"
			        	in++;
			        }
			        if(out==1) {
			        	outi=i-1;//storing the index of ">"
			        	out++;
			        }
			    }

			    arg[i]=NULL;
			    i--;

			    //in case of background command directive
			    if(arg[i][strlen(arg[i])-1]=='&' && backGround==0 && strlen(arg[i])>1 ) {
			    	char *temp = strtok(arg[i],"&");
			    	arg[i]=temp;
					backGround=1;
				}
			    
			    if(strncmp(arg[i],"&",1)==0 && backGround==0) {
			    	backGround=1;
			    	arg[i]=NULL;
				}

			    int ifd=0,ofd=1;
			    if(in>2 || out>2) {//when user types more than one ">" or "<" in one command
			    	printf("Incorrect input/output file format\n");
			    	continue;
			    }
			    else if (in==2 || out==2) {
			    	int mini=(ini<outi)?ini:outi;
			    	arg[mini]=NULL;

			    	if (in==2) {
			  		    strcpy(infile, arg[ini+1]); //storing input filename in infile
			    		/* Open input file descriptor */
				    	ifd = open(infile, O_RDONLY);
						if (ifd < 0) {
						  perror("open ERROR ");
						  printf("Cannot open input file \"%s\"\n",infile);
						  continue;
						}
					}

					if (out==2){
				       	strcpy(outfile, arg[outi+1]); //storing output filename in outfile
						/* Open output file descriptor */
	   					/* The file is created in the mode rw-r--r-- (644) */
						ofd = open(outfile, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
						if (ofd < 0) {
						  perror("open ERROR ");
						  printf("Cannot open output file \"%s\"\n",outfile);
						  continue;
						}
					}
				}

			    int id,status;
				//create a child process
				id=fork();
				//if child process
				if(id==0)
				{
					if(in==2) {
						close(0);	 	/* Close the file descriptor for stdin */
						dup(ifd); 		/* Duplicate ifd at the lowest-numbered unused descriptor */
						close(ifd);		/* ifd is no longer needed */
					}

					if (out==2) {
						close(1);	 	/* Close the file descriptor for stdout */
						dup(ofd); 		/* Duplicate ofd at the lowest-numbered unused descriptor */
						close(ofd);		/* ofd is no longer needed */
					}
				
					//executing files
		            execvp(arg[0], arg); //executing the mentioned executable
		            perror("execvp ERROR ");
		            continue;
				}
				
				//fork ERROR
				else if (id==-1) {
					printf("fork ERROR ");
					continue;
				}
				
				//parent process
				else
				{
					if (backGround==0) wait(&status); //if not a background program, parent waits for child to finish
				}

			}
		}

	}
	return 0;
}