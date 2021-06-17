#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#define MESSAGESIZE 4096

int main(int argc, char** argv){

	int bytesRead = 0;
	char string[MESSAGESIZE];

    while((bytesRead = read(STDIN_FILENO, string, MESSAGESIZE)) > 0){
    	if(string[bytesRead - 1] == '\n') string[--bytesRead] = 0;

    	int client_server_fifo = open("client_server_fifo", O_WRONLY);
    	int server_client_fifo = open("server_client_fifo", O_RDONLY);
	    write(client_server_fifo, string, bytesRead);
	    close(client_server_fifo);
	    printf("%d\n", bytesRead);
	    while((bytesRead = read(server_client_fifo, string, MESSAGESIZE)) > 0){
            write(STDOUT_FILENO, string, bytesRead);
	    }
	    close(server_client_fifo);
	}

    return 0;
}