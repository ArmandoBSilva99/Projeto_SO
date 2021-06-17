#ifndef READ_CONFIG_H
#define READ_CONFIG_H

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

ssize_t myreadln(int fildes, void* buf, size_t nbyte);
void fillAurray(char * line, int i);
int readc(int fd, char* c);

#endif