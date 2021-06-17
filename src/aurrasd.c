#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include "readconfig.h"

#define MESSAGESIZE 4096

#define FINISHED 0
#define EXECUTING 1
#define TERMINATED 2
#define TERMINACTIVE 3
#define TERMTEXEC 4
#define WAITING 5
#define ERROR 255

char* filtros_array[2048];
int exitStatus[2048];
int exitMatrix[2048][3];
int execTimes[2048];
char commTimes[2048];
int monitors[2048];
int pids[2048][3];
int numPids[2048];
int lastProcess = 0;
int tExec = -1;
int tInac = -1;

void child_handler(int sig){
    int status;
    pid_t pid;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0){
        printf("ENTREI\n");
        printf("PID A SAIR -> %d\n", pid);
        for(int i = 0; i < lastProcess; i++){
            for(int j = 0; j < 3; j++){
                if(pids[i][j] == pid){
                    exitMatrix[i][j] = FINISHED;
                    printf("PID TERMINADO -> %d\n", pids[i][j]);
                    if(exitStatus[i] == EXECUTING && exitMatrix[i][0] == FINISHED && exitMatrix[i][1] == FINISHED && exitMatrix[i][2] == FINISHED){
                        exitStatus[i] = FINISHED;
                        break;
                    }
                }
            }
        }
    }
}

void alarm_handler(int sig){

}

void exec_filtros(char* filtro){

    if (strcmp(filtro,"eco") == 0)        
        execl("aurrasd-filters/aurrasd-echo","aurrasd-echo", NULL);
    if (strcmp(filtro,"alto") == 0)
        execl("aurrasd-filters/aurrasd-gain-double","aurrasd-gain-double", NULL);
    if (strcmp(filtro,"baixo") == 0)
        execl("aurrasd-filters/aurrasd-gain-half","aurrasd-gain-half", NULL);
    if (strcmp(filtro,"rapido") == 0)
        execl("aurrasd-filters/aurrasd-tempo-double","aurrasd-tempo-double", NULL);
    if (strcmp(filtro,"lento") == 0)
        execl("aurrasd-filters/aurrasd-tempo-half","aurrasd-tempo-half", NULL);
}


void fillConfig(){
    int fd;

    if((fd = open("../etc/aurrasd.conf", O_RDONLY, 0644)) == -1){
        perror("File [/etc/aurrasd.config] doesn't exist!\n");
        exit(-1);
    }

    char* line = malloc(MAX_BUF);
    int i=0;

    while((myreadln(fd, line, 30) > 0) && i < 5){
        fillAurray(line, i); 
        i++;
    }

    close(fd);
}

void fillAurray(char * line, int i){
    aurray[i].name = strdup(strtok(line, " "));
    aurray[i].exe = strdup(strtok(NULL, " "));
    aurray[i].max = atoi(strtok(NULL, "\n"));
}


ssize_t myreadln(int fildes, void* buf, size_t nbyte){
    ssize_t size = 0;
    char c;
    char* buff = (char*)buf;
    while (size < nbyte && read(fildes, &c, 1) > 0) {
        if (c == '\0')
            return size;
        buff[size++] = c;
        if (c == '\n')
            return size;
    }
    return size;
}

char* writeConfig(){
    char ret[1024];
    for(int i = 0; i < 5; i++){
        char buf[30]; 
        sprintf(buf, "%s %s %d\n", aurray[i].name,aurray[i].exe,aurray[i].max);
        strcat(ret,buf);
    }
    return strdup(ret);    
}

int checkConfig(char** filters){
    for (int i = 0; i < 3; i++)
        for(int j = 0; j < 5; j++) 
            if ((strcmp(filters[i],aurray[j].name) == 0) && aurray[j].max == 0)
                return 0;
    return 1;
}

int main(){
    
    fillConfig();

    mkfifo("client_server_fifo", 0644);
    mkfifo("server_client_fifo", 0644);
    signal(SIGCHLD, child_handler);
    while(1){

        char* buffer = calloc(MESSAGESIZE, sizeof(char));
        int client_server_fifo = open("client_server_fifo", O_RDONLY);
        int server_client_fifo = open("server_client_fifo", O_WRONLY);
        read(client_server_fifo, buffer, MESSAGESIZE);
        printf("%s\n", buffer);
        

        char *array[100];
        int io = 0;

        array[io] = strtok(buffer," ");

        while(array[io] != NULL){
            array[++io] = strtok(NULL," ");
        }


        char** message_filters = malloc(sizeof(char*) * (io-3));

        for (int i = 3, j = 0; i < io-3; i++,j++)
                message_filters[j] = strdup(array[i]); 

        for (int i = 0; i < io-3; i++)
                printf("[%d] -> %s\n", i, message_filters[i]);      

        if(checkConfig(message_filters) == 0){
            write(server_client_fifo, "Pending...\n", 11);
            exitStatus[lastProcess] = WAITING;
        }
        else
            exitStatus[lastProcess] = EXECUTING; 
        
        if(strncmp(buffer,"transform",9) == 0 && (exitStatus[lastProcess] == EXECUTING)){
            
            processes[lastProcess].message = strdup(buffer);
            printf("Message: %s\n", processes[lastProcess].message);
            printf("PROCESSO -> %s\n", processes[lastProcess].message);
        

            for(int i = 0; i < 3; i++)
                exitMatrix[lastProcess][i] = EXECUTING;
            
            processes[lastProcess].filtros = malloc(sizeof(char*) * (io-3));
            
            for (int j = 0; j < io-3;j++)
                processes[lastProcess].filtros[j] = strdup(message_filters[j]);

            for (int i = 0; i < io-3; i++)
                printf("[%d] -> %s\n", i,processes[lastProcess].filtros[i]);        

            io--;

            int in = open(array[1], O_RDONLY,0644);
            int out = open(array[2], O_WRONLY | O_TRUNC | O_CREAT,0644);

            char message[100];
            sprintf(message, "\nnova tarefa #%d\n\n", lastProcess + 1);
            write(server_client_fifo, message, strlen(message));
            close(server_client_fifo);
            
            int pid;
            int beforePipe = in;
            int afterPipe[2];

            if(io > 3){
                int acc = 0;
                dup2(in, 0);
                close(in);
                for (int i = 3; i <= io; i++) {
                    if (i < io)
                        pipe(afterPipe);
                    if ((pid = fork()) == 0) {
                        if (i > 3) {
                            dup2(beforePipe, 0);
                            close(beforePipe);
                        }
                        if (i < io) {
                            dup2(afterPipe[1], 1);
                            close(afterPipe[0]);
                            close(afterPipe[1]);
                        }
                        if(i == io){
                            dup2(out,1);
                            close(out);
                        }
                        exec_filtros(array[i]);
                        _exit(0);
                    }
                    if (i < io)
                        close(afterPipe[1]);
                    if (i > 3)
                        close(beforePipe);
                    beforePipe = afterPipe[0];
                    pids[lastProcess][acc] = pid;
                    processes[lastProcess].pid = pid;
                    acc++;

                }
            } else {
            
                if((pid = fork()) == 0) {
                    dup2(out, 1);
                    dup2(in, 0);
                    close(in);
                    close(out);
                    
                    exec_filtros(array[3]);
                    _exit(0);
                }else{
                    pids[lastProcess][0] = pid;
                    printf("PIDS[%d][0] -> %d\n",lastProcess,pids[lastProcess][0]);
                }
            }
            lastProcess++;
            close(in);
            close(out);
        }
        if(strncmp(buffer, "status", 6) == 0) {

            char message[1024];
            int empty = 1;
            char* conf = writeConfig();
            for(size_t i = 0; i < lastProcess; i++) {
                if(exitStatus[i] == EXECUTING) {
                    empty = 0;
                    sprintf(message, "#%zu: %s\n", i+1, processes[i].message);
                    strcat(message, conf);
                    write(server_client_fifo, message, strlen(message));
                }
                if(exitStatus[i] == FINISHED) {
                    empty = 0;
                    sprintf(message, "->->#%zu: %s\n", i+1, processes[i].message);
                    strcat(message, conf);
                    write(server_client_fifo, message, strlen(message));
                }
            }
            if(empty) {
                strcpy(message, "Não há tarefas em execução.\n\n");
                write(server_client_fifo, message, strlen(message));
            }
            close(server_client_fifo);
        }
    }
    return 0;
}