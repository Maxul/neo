#include "com.h"
#include "list.h"
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <X11/Xlib.h>

extern Node *head;
extern Queue *pq;

extern char *ram_start;

extern pthread_mutex_t mutex_queue;
extern pthread_mutex_t mutex_link;
extern pthread_mutex_t mutex_closenotify;
extern pthread_mutex_t mutex_fd;

void *recv_cmd(void *unused)
{
    char *pcmd;
    char c;
    char *cmd_flag = ram_start + 0xff;
    int64_t len;
    FILE *fpipe;

    for (;;)
    {
        pthread_mutex_lock(&mutex_fd);//P
        c = *cmd_flag;
        pthread_mutex_unlock(&mutex_fd);//V
        if ('1' == c)
        {
            pthread_mutex_lock(&mutex_fd);//P
            *cmd_flag = '0';
            pthread_mutex_unlock(&mutex_fd);//V

            len = *(int64_t *)(cmd_flag + 1);
            pcmd = malloc(len + 1);
            memcpy(pcmd, cmd_flag + 1 + sizeof(int64_t), len);
            pcmd[len] = 0;
            //printf("len %ld cmd %s\n", len, pcmd);

            fpipe = popen(pcmd, "r");
            //pclose(fpipe); /* we don't have to close the pipe */
            free(pcmd);
        }
        sleep(1);
    }

    return NULL;
}

void *recv_events(void *unused)
{
    char c;
    char *event_flag = (ram_start + 0x8);
    Item item_tmp;

    for (;;)
    {
        pthread_mutex_lock(&mutex_fd);//P
        c = *event_flag;
        pthread_mutex_unlock(&mutex_fd);//V

        if (c != '-') // !='-' stands for recieving new event
        {
            memcpy((void *)&item_tmp, ram_start + 200, sizeof(Item));

            pthread_mutex_lock(&mutex_link);
            if (-1 == FindByValue(head, item_tmp.source_wid))
            {
                printf("uh oh %lx\n", item_tmp.source_wid);
                send_notify(item_tmp.source_wid);
            }
            pthread_mutex_unlock(&mutex_link);

            item_tmp.type = c; // restore

            pthread_mutex_lock(&mutex_queue);
            EnQueue(pq, item_tmp);
            pthread_mutex_unlock(&mutex_queue);

            pthread_mutex_lock(&mutex_fd);//P
            *event_flag = '-';
            pthread_mutex_unlock(&mutex_fd);//V
        }
    }
}

void send_notify(Window win)
{
    char c;
    char *close_flag = ram_start + 0xa;

    pthread_mutex_lock(&mutex_closenotify);
    for (;;)
    {
        pthread_mutex_lock(&mutex_fd);//P
        c = *close_flag;
        pthread_mutex_unlock(&mutex_fd);//V

        if ('0' == c)//qemu ready to receive
        {
            printf("%lx should be closed\n", win);

            pthread_mutex_lock(&mutex_fd);//P
            *(int *)(close_flag + 1) = (int)win;
            *close_flag = '1'; //'1' implies backend has sent notify
            pthread_mutex_unlock(&mutex_fd);//V

            pthread_mutex_unlock(&mutex_closenotify);
            return;
        }
    }
}

