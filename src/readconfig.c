#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "readconfig.h"


int main(int argc, char* argv[]){
    int fd;

    if((fd = open("../etc/aurrasd.conf", O_RDONLY, 0644)) == -1){
        perror("File [/etc/aurrasd.config] doesn't exist!\n");
        exit(-1);
    }

    char* line = malloc(MAX_BUF);
    int bytes_read;
    int i=0;

    while((myreadln(fd, line, 30) > 0) && i < 5){
        fillAurray(line, i); 
        i++;
    }

    close(fd);
    return 0;
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