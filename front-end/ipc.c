#include "ipc.h"
#include "queue.h"

#include <pthread.h>

int set_semvalue(int sem_id)
{
    union semun sem_union;

    sem_union.val = 1;
    if(semctl(sem_id, 0, SETVAL, sem_union) == -1) return 0;
    return 1;
}

int semaphore_p(int sem_id)
{
    struct sembuf sem_b;

    sem_b.sem_num = 0;
    sem_b.sem_op = -1;
    sem_b.sem_flg = SEM_UNDO;
    if(semop(sem_id, &sem_b, 1) == -1)
    {
        printf("semaphore_p failed\n");
        return 0;
    }
    return 1;
}

int semaphore_v(int sem_id)
{
    struct sembuf sem_b;

    sem_b.sem_num = 0;
    sem_b.sem_op = 1;
    sem_b.sem_flg = SEM_UNDO;
    if(semop(sem_id, &sem_b, 1) == -1)
    {
        printf("semaphore_v failed\n");
        return 0;
    }
    return 1;
}

extern char *mem;

extern Queue *pq_close;

extern pthread_mutex_t mutex_close_queue;

/* add close_notify to queue*/
void* close_notify_queue(void* arg)
{
    Item item_tmp;
    int flag;

    for (;;)
    {
        flag = 0;
        semaphore_p(sem_id);
        if (*((char *)(mem + 1024 * 1024 * 4 + sizeof(int))) == '1')
        {
            item_tmp.source_wid = *((int *)(mem + 1024 * 1024 * 4 + sizeof(int) + 1));
            *((char *)(mem + 1024 * 1024 * 4 + sizeof(int))) = '0';
            semaphore_v(sem_id);
            flag = 1;
            printf("0x%lx closed\n", item_tmp.source_wid);

            pthread_mutex_lock(&mutex_close_queue);
            EnQueue(pq_close, item_tmp);
            pthread_mutex_unlock(&mutex_close_queue);
        }
        if (flag == 0)
            semaphore_v(sem_id);
    }
}

