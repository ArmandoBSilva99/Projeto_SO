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
#define MAXCLIENTS 10

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
    int pidFifo;
} *Process;

struct process processes[2048];

struct aurrasdconfig aurray[5];

char* filtros_array[2048];
int exitStatus[2048];
int exitMatrix[2048][3];
int pids[2048][3];
int lastProcess = 0;

char* fifos[MAXCLIENTS];
int lastFifo = 0;

char aurrasd_filters[30];
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
int sizeFilters(char ** filters)
{
    int size = 0;
    while(filters[size]) size++;
    return size;
}

void aumentaFiltros(char** filters)
{
    int size = sizeFilters(filters);
    for (int i = 0; i < size; i++)
        for(int j = 0; j < 5; j++) 
            if ((strcmp(filters[i],aurray[j].name) == 0))
                aurray[j].max++;
}

void diminuiFiltros(char** filters)
{
    int size = sizeFilters(filters);
    for (int i = 0; i < size; i++)
        for(int j = 0; j < 5; j++) 
            if ((strcmp(filters[i],aurray[j].name) == 0))
                aurray[j].max--;
}

void child_handler(int sig){
    int status;
    pid_t pid;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0){
        for(int i = 0; i < lastProcess; i++){
            for(int j = 0; j < 3; j++){
                if(pids[i][j] == pid){
                    exitMatrix[i][j] = FINISHED;
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

    char echo[100];
    sprintf(echo,"%s/aurrasd-echo",aurrasd_filters);

    char doubl[100]; 
    sprintf(doubl,"%s/aurrasd-gain-double",aurrasd_filters);

    
    char half[100]; 
    sprintf(half,"%s/aurrasd-gain-half",aurrasd_filters);

    char temp_double[100]; 
    sprintf(temp_double,"%s/aurrasd-tempo-double",aurrasd_filters);

    
    char temp_half[50]; 
    sprintf(temp_half,"%s/aurrasd-tempo-half",aurrasd_filters);

    if (strcmp(filtro,"echo") == 0)        
        execl(echo,"aurrasd-echo", NULL);
    if (strcmp(filtro,"alto") == 0)
        execl(doubl,"aurrasd-gain-double", NULL);
    if (strcmp(filtro,"baixo") == 0)
        execl(half,"aurrasd-gain-half", NULL);
    if (strcmp(filtro,"rapido") == 0)
        execl(temp_double,"aurrasd-tempo-double", NULL);
    if (strcmp(filtro,"lento") == 0)
        execl(temp_double,"aurrasd-tempo-half", NULL);
}



void transform()
{
    char *array[100];
    int io = 0;
        
    char * buffer = strdup(processes[lastProcess].message);
    printf("dupped\n");
    array[io] = strtok(buffer," ");

    int size = sizeFilters(processes[lastProcess].filtros);
    while(array[io] != NULL){
        printf("array[io]->%s\n",array[io]);
        array[++io] = strtok(NULL," ");
    }

    for(int i = 0; i < size; i++)
        exitMatrix[lastProcess][i] = EXECUTING;

    diminuiFiltros(processes[lastProcess].filtros);      
    printf("Diminuiu Filtros\n");
    io--;


    int in = open(array[1], O_RDONLY,0644);
    int out = open(array[2], O_WRONLY | O_TRUNC | O_CREAT,0644);

    char message[100];
    sprintf(message, "\nProcessing #%d\n\n", lastProcess + 1);
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
        }
    }
    
    lastProcess++;
    close(in);
    close(out);
}



int checkConfig(char** filters){
    int size = sizeFilters(filters);
    for (int i = 0; i < size; i++)
        for(int j = 0; j < 5; j++) 
            if ((strcmp(filters[i],aurray[j].name) == 0) && aurray[j].max == 0)
                return 0;
    return 1;
}


void alarm_handler(int sig)
{
    for(int i = 0; i <= lastProcess; i++){
        if((exitStatus[i] == WAITING) && checkConfig(processes[i].filtros)){
            exitStatus[i] = EXECUTING;
            //server_client_fifo = open("server_client_fifo", O_WRONLY);
            close(server_client_fifo);
            char connect[30];
            sprintf(connect,"%s%d","server_client_fifo_", processes[i].pidFifo);
            server_client_fifo = open(connect, O_WRONLY);
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

void connectClient(char * connectMessage)
{
    char *array[5];
    int i = 0;

    array[i] = strtok(connectMessage," ");

    while(array[i] != NULL)
        array[++i] = strtok(NULL," ");
            
    if(lastFifo < MAXCLIENTS - 1)
    {
        fifos[lastFifo] = strdup(array[1]);
        lastFifo++;
    }

}


int getPidFromString(char * pidMessage)
{
    char *array[5];
    int i = 0;

    array[i] = strtok(pidMessage," ");

    while(array[i] != NULL)
        array[++i] = strtok(NULL," ");

    return atoi(array[1]);
}


int main(int args, char* argv[])
{
    if (args == 3){
        fillConfig(argv[1]);

        strcpy(aurrasd_filters,argv[2]);
    }
    else {
        write(1,"Argumentos errados! Tente:\n./aurrasd config-filename filters-folder\n",68);
        exit(-1);
    }

    mkfifo("client_server_fifo", 0644);

    signal(SIGCHLD, child_handler);
    signal(SIGALRM, alarm_handler);


    while(1){

        char* buffer = calloc(MESSAGESIZE, sizeof(char));
        int client_server_fifo = open("client_server_fifo", O_RDONLY); 
        read(client_server_fifo, buffer, MESSAGESIZE);
        
        char *array[100];
        int io = 0;
        
        if(strncmp(buffer,"connect",7) == 0)
        {
            connectClient(buffer);
            write(1,"connected\n",10);
            close(client_server_fifo);
        }


        int pidFifo = -1;
        if(strncmp(buffer,"pid",3) == 0)
        {
            char scf[30];
            pidFifo = getPidFromString(buffer);
            sprintf(scf,"%s%d","server_client_fifo_", pidFifo);
            server_client_fifo = open(scf, O_WRONLY);
            free(buffer);
            buffer = calloc(MESSAGESIZE, sizeof(char));
            read(client_server_fifo, buffer, MESSAGESIZE);
        }            

        
        if(strncmp(buffer,"transform",9) == 0)
        {
            processes[lastProcess].message = strdup(buffer);

            array[io] = strtok(buffer," ");

            while(array[io] != NULL){
                array[++io] = strtok(NULL," ");
            }
            io--;
            char** message_filters = (char**) malloc(3 * sizeof(char*));

            for (int j = 0; j < io-3; j++)
                message_filters[j] = strdup(array[j+3]); 

            printf("HEHE\n");
            processes[lastProcess].filtros = malloc(sizeof(char*) * (io-3));
                
            for (int j = 0; j < io-3;j++)
                processes[lastProcess].filtros[j] = strdup(message_filters[j]);

        
            if(checkConfig(processes[lastProcess].filtros) == 0){
                processes[lastProcess].pidFifo = pidFifo;
                exitStatus[lastProcess] = WAITING;
                alarm(1);           
            }
            else
                exitStatus[lastProcess] = EXECUTING;
               
        }
        
        if(processes[lastProcess].message && strncmp(processes[lastProcess].message,"transform",9) == 0 && (exitStatus[lastProcess] == EXECUTING)){
            printf("executing transform\n");
            transform();
        
        if(strncmp(buffer, "status", 6) == 0) 
        {
            char message[1024];
            int empty = 1;
            char* conf;
            conf = writeConfig();
            for(size_t i = 0; i < lastProcess; i++) {
                if(exitStatus[i] == EXECUTING) {
                    empty = 0;
                    sprintf(message, "#%zu: %s\n", i+1, processes[i].message);
                    write(server_client_fifo, message, strlen(message));
                }
            }
            if(empty) {
                strcpy(message, "Não há tarefas em execução.\n\n");
                write(server_client_fifo, message, strlen(message));
            }
            char * aux = strdup(conf);
            sprintf(conf,"%spid: %d\n",aux,pidFifo);
            free(aux);
            write(server_client_fifo, conf, strlen(conf));
            free(conf);
            close(server_client_fifo);
        }
    }
    return 0;
}