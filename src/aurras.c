#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#define MESSAGESIZE 4096

int main(int argc, char** argv){

	int pid_client = getpid();
	int bytesRead = 0;
	char string[MESSAGESIZE];
	
	char csf[30];
	char scf[30]; 
	sprintf(csf,"%s","client_server_fifo");
	sprintf(scf,"%s%d","server_client_fifo_", pid_client);

    mkfifo(scf, 0644);

	int client_server_fifo = open(csf, O_WRONLY);
    //int server_client_fifo = open(scf, O_RDONLY);

    char connectMessage[30];
    sprintf(connectMessage,"%s%d","connect ", pid_client);

    char pidMessage[30];
    sprintf(pidMessage,"%s%d","pid ", pid_client);

    write(client_server_fifo,connectMessage,MESSAGESIZE);

    while((bytesRead = read(STDIN_FILENO, string, MESSAGESIZE)) > 0){
    	if(string[bytesRead - 1] == '\n') string[--bytesRead] = 0;

    	int client_server_fifo = open(csf, O_WRONLY);
    	write(client_server_fifo, pidMessage, MESSAGESIZE);
	    write(client_server_fifo, string, bytesRead);
	    close(client_server_fifo);

    	int server_client_fifo = open(scf, O_RDONLY);
	    while((bytesRead = read(server_client_fifo, string, MESSAGESIZE)) > 0)
	    {          
            write(STDOUT_FILENO, string, bytesRead);
	    }
	    close(server_client_fifo);
	}

    return 0;
}