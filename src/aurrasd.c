#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#define MESSAGESIZE 4096

#define FINISHED 0
#define EXECUTING 1
#define TERMINATED 2
#define TERMINACTIVE 3
#define TERMTEXEC 4
#define WAITING 5
#define ERROR 255
#define MAX_BUF 1024

//struct aurrasconfig
struct aurrasdconfig {
    char* name;
    char* exe;
    int max;
} *AurrasdConfig;

//struct processos
struct process{
    int pid;
    char* message;
    char** filtros;
} *Process;

struct process processes[2048];

struct aurrasdconfig aurray[5];

//Variáveis para realn
char read_buffer[2048];
int read_buffer_pos = 0;    
int read_buffer_end = 0;
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
char* path_filters;

//pipe
int server_client_fifo;

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

void aumentaFiltros(char** filters)
{
    for (int i = 0; i < 3 && filters[i]; i++)
        for(int j = 0; j < 5 && filters[i][j]; j++) 
            if ((strcmp(filters[i],aurray[j].name) == 0))
                aurray[j].max++;
}

void diminuiFiltros(char** filters)
{
    for (int i = 0; i < 3 && filters[i]; i++)
        for(int j = 0; j < 5 && filters[i][j]; j++) 
            if ((strcmp(filters[i],aurray[j].name) == 0))
                aurray[j].max--;
}

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
                        aumentaFiltros(processes[i].filtros);
                        break;
                    }
                }
            }
        }
    }
}

void exec_filtros(char* filtro){


    if (strcmp(filtro,"eco") == 0){
        char* filters = malloc(sizeof(char) * 100);
        sprintf(filters, "%s/aurrasd-echo", path_filters);
        write(1,filters, 40);
        execl(filters,"aurrasd-echo", NULL);
    }    
    if (strcmp(filtro,"alto") == 0){
        char* filters = malloc(sizeof(char) * 100);
        sprintf(filters, "%s/aurrasd-gain-double", path_filters);
        printf("filter -> %s\n", filters);
        execl(filters,"aurrasd-gain-double", NULL);
    }    
    if (strcmp(filtro,"baixo") == 0){
        char* filters = malloc(sizeof(char) * 100);
        sprintf(filters, "%s/aurrasd-gain-half", path_filters);
        printf("filter -> %s\n", filters);
        execl(filters,"aurrasd-gain-half", NULL);
    }    
    if (strcmp(filtro,"rapido") == 0){
        char* filters = malloc(sizeof(char) * 100);
        sprintf(filters, "%s/aurrasd-tempo-double", path_filters);
        printf("filter -> %s\n", filters);
        execl(filters,"aurrasd-tempo-double", NULL);
    }    
    if (strcmp(filtro,"lento") == 0){
        char* filters = malloc(sizeof(char) * 100);
        sprintf(filters, "%s/aurrasd-tempo-half", path_filters);
        printf("filter -> %s\n", filters);
        execl(filters,"aurrasd-tempo-half", NULL);
    }    
}

void transform()
{
    char *array[100];
    int io = 0;

    printf("Message: %s\n", processes[lastProcess].message);
    printf("PROCESSO -> %s\n", processes[lastProcess].message);
        
    char * buffer = strdup(processes[lastProcess].message);
    printf("dupped\n");
    array[io] = strtok(buffer," ");

    while(array[io] != NULL){
        printf("array[io]->%s\n",array[io]);
        array[++io] = strtok(NULL," ");
    }

    for(int i = 0; i < 3; i++)
        exitMatrix[lastProcess][i] = EXECUTING;

    diminuiFiltros(processes[lastProcess].filtros);      
    printf("Diminuiu Filtros\n");
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

    if(io > 3)
    {
        printf("\nHELLO\n");
        int acc = 0;
        dup2(in, 0);
        close(in);
        for (int i = 3; i <= io; i++) 
        {
            if (i < io)
                pipe(afterPipe);
            if ((pid = fork()) == 0) 
            {   
                printf("Son says HI!\n");
                if (i > 3) 
                {
                    dup2(beforePipe, 0);
                    close(beforePipe);
                }
                if (i < io) 
                {
                    dup2(afterPipe[1], 1);
                    close(afterPipe[0]);
                    close(afterPipe[1]);
                }
                if(i == io)
                {
                    dup2(out,1);
                    close(out);
                }
                
                exec_filtros(array[i]);
                exit(0);
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
    } 
    else 
    {
        if((pid = fork()) == 0) 
        {   
            printf("Im in\narray[3]: %s\n", array[3]);
            dup2(out, 1);
            dup2(in, 0);
            close(in);
            close(out);
                    
            exec_filtros(array[3]);
            _exit(0);
        }
        else
        {
            pids[lastProcess][0] = pid;
            printf("PIDS[%d][0] -> %d\n",lastProcess,pids[lastProcess][0]);
        }
    }
    
    lastProcess++;
    close(in);
    close(out);
}


int checkConfig(char** filters){
    for (int i = 0; i < 3 && filters[i]; i++)
        for(int j = 0; j < 5 && filters[i][j]; j++) 
            if ((strcmp(filters[i],aurray[j].name) == 0) && aurray[j].max == 0)
                return 0;
    return 1;
}


void alarm_handler(int sig)
{
    for(int i = 0; i <= lastProcess; i++){
        printf("FOR:EXIT STATUS -> %d -> %d\n",i, exitStatus[i]);
        if((exitStatus[i] == WAITING) && checkConfig(processes[i].filtros)){
            exitStatus[i] = EXECUTING;
            printf("EXIT STATUS -> %d -> %d",i, exitStatus[i]);
            transform();
        }
    }

    alarm(1);
}



void fillAurray(char * line, int i){
    aurray[i].name = strdup(strtok(line, " "));
    aurray[i].exe = strdup(strtok(NULL, " "));
    aurray[i].max = atoi(strtok(NULL, "\n"));
}


void fillConfig(char* path){
    int fd;
    if((fd = open(path, O_RDONLY, 0644)) == -1){
        perror("File doesn't exist!\n");
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


char* writeConfig(){
    char* ret = malloc(sizeof(char)*30*5);
    for(int i = 0; i < 5; i++){
        char buf[30]; 
        sprintf(buf, "%s %s %d\n", aurray[i].name,aurray[i].exe,aurray[i].max);
        strcat(ret,buf);
    }
    return ret;    
}

void checkfilters(char* path){
    int fd_echo;
    int fd_double;
    int fd_half;
    int fd_temp_double;
    int fd_temp_half;
    
    char* newpathecho = strdup(path);
    char* newpathdouble = strdup(path);
    char* newpathhalf = strdup(path);
    char* newpathtempdouble = strdup(path);
    char* newpathtemphalf = strdup(path);
       
    char* echo = strcat(newpathecho,"/aurrasd-echo");
    char* doubl = strcat(newpathdouble,"/aurrasd-gain-double");
    char* half = strcat(newpathhalf,"/aurrasd-gain-half");
    char* temp_double = strcat(newpathtempdouble,"/aurrasd-tempo-double");
    char* temp_half = strcat(newpathtemphalf,"/aurrasd-tempo-half");

    
    if(((fd_echo = open(echo, O_RDONLY, 0644)) == -1) && ((fd_double = open(doubl, O_RDONLY, 0644)) == -1) && ((fd_half = open(half, O_RDONLY, 0644)) == -1) 
        && ((fd_temp_double = open(temp_double, O_RDONLY, 0644)) == -1) && ((fd_temp_half = open(temp_half, O_RDONLY, 0644)) == -1)){
        perror("Wrong filter file!\n");
        exit(-1);
    }


    close(fd_echo);
    close(fd_double);
    close(fd_half);
    close(fd_temp_double);
    close(fd_temp_half);
}


int main(int args, char* argv[]){
    path_filters = strdup(argv[2]);
    printf("\npath filters -> %s\n", path_filters);
    if (args == 3){
        fillConfig(argv[1]);

        checkfilters(path_filters);    
    }
    else {
        write(1,"Argumentos errados! Tente:\n./aurrasd config-filename filters-folder\n",68);
        exit(-1);
    }

    mkfifo("client_server_fifo", 0644);
    mkfifo("server_client_fifo", 0644);
    signal(SIGCHLD, child_handler);
    signal(SIGALRM, alarm_handler);
    while(1){

        char* buffer = calloc(MESSAGESIZE, sizeof(char));
        int client_server_fifo = open("client_server_fifo", O_RDONLY); 
        server_client_fifo = open("server_client_fifo", O_WRONLY);
        printf("AQUI\n");
        read(client_server_fifo, buffer, MESSAGESIZE);
        printf("%s\n", buffer);
        
        char *array[100];
        int io = 0;
        
        printf("1\n");
        if(strncmp(buffer,"transform",9) == 0)
        {
            printf("2\n");

            processes[lastProcess].message = strdup(buffer);

            array[io] = strtok(buffer," ");

            while(array[io] != NULL){
                array[++io] = strtok(NULL," ");
            }

            char** message_filters = (char**) malloc(3 * sizeof(char*));

            for (int i = 0; i < io; i++)
                printf("array[%d] -> %s\n", i,array[i]);
            
            for (int j = 0; j < 3 && array[j+3]; j++)
                message_filters[j] = strdup(array[j+3]); 

            printf("HEHE\n");
            processes[lastProcess].filtros = malloc(sizeof(char*) * (io-3));
                
            for (int j = 0; j < io-3 && message_filters[j];j++)
                processes[lastProcess].filtros[j] = strdup(message_filters[j]);     
            
            printf("HELP\n");
            if(checkConfig(processes[lastProcess].filtros) == 0){
                printf("bruv\n");
                exitStatus[lastProcess] = WAITING;
                alarm(1);           
            }
            else
                exitStatus[lastProcess] = EXECUTING;
               
        }
        
        if(processes[lastProcess].message && strncmp(processes[lastProcess].message,"transform",9) == 0 && (exitStatus[lastProcess] == EXECUTING)){
            printf("executing transform\n");
            transform();
        }
        if(strncmp(buffer, "status", 6) == 0) {

            char message[1024];
            int empty = 1;
            char* conf;
            conf = writeConfig();
            for(size_t i = 0; i < lastProcess; i++) {
                if(exitStatus[i] == EXECUTING) {
                    empty = 0;
                    printf("Message: %s\n", processes[i].message);
                    sprintf(message, "#%zu: %s\n", i+1, processes[i].message);
                    //strcat(message, conf);
                    write(server_client_fifo, message, strlen(message));
                }
                if(exitStatus[i] == FINISHED) {
                    empty = 0;
                    sprintf(message, "->->#%zu: %s\n", i+1, processes[i].message);
                    //strcat(message, conf);
                    write(server_client_fifo, message, strlen(message));
                }
            }
            if(empty) {
                printf("Entrou no empty\n");
                strcpy(message, "Não há tarefas em execução.\n\n");
                write(server_client_fifo, message, strlen(message));
            }
            write(server_client_fifo, conf, strlen(conf));
            free(conf);
            close(server_client_fifo);
        }
    }
    return 0;
}