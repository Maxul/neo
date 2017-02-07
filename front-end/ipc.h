#ifndef SERVER_SEM_H
#define SERVER_SEM_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

union semun {
    int val;
    unsigned short *array;
    struct semid_ds *buf;
};

int set_semvalue(int semid);
int semaphore_p(int semid);
int semaphore_v(int semid);
int sem_id;

void* close_notify_queue(void* arg);

#endif
