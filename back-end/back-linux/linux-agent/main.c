#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>

#include "queue.h"
#include "list.h"
#include "com.h"
#include "emulate_event.h"
#include "manipulate_win.h"
#include "print_win.h"
#include "memctl.h"

#define MAGIC "TRUST"

/* guest RAM */
char *ram_start;

/* kernel memory */
char *mem;

int ErrorFlag = 0;

uint64_t offset;
Display *dpy = NULL;

Queue *pq;
/* head is all we've maintained so far */
Node *head;
/* head_new is what's new on desktop */
Node *head_new;

pthread_mutex_t mutex_mem;
pthread_mutex_t mutex_queue;
pthread_mutex_t mutex_link;
pthread_mutex_t mutex_closenotify;
pthread_mutex_t mutex_fd;

static int bad_window_handler(Display *dsp, XErrorEvent *err)
{
    char buff[256];

    XGetErrorText(dsp, err->error_code, buff, sizeof(buff));
    printf("X Error %s\n", buff);
    ErrorFlag = 1;
    return 0;
}

int main(int argc, char** argv)
{
    puts("Neo man is ready!");
    
    pthread_t thread_window;
    pthread_t thread_cmd;

    pthread_t thread_recv_event;
    pthread_t thread_emulate_event;

    int major_opcode, first_event, first_error;

    /* Check Xrender Support */
    XInitThreads();
    dpy = XOpenDisplay(":0.0");
    if (XQueryExtension(dpy, RENDER_NAME, &major_opcode, &first_event, &first_error) == False)
    {
        fprintf(stderr, "No Xrender-Support\n");
        return 0;
    }

    /* open RAM and obtain the kmem's offset in of RAM */
    ram_start = openRAM();
    printf("Waiting for kernel memory allocating.\n");
    while (memcmp(ram_start, MAGIC, sizeof(MAGIC)) != 0)
    {
        sleep(1);
        printf(" .");
        fflush(stdout);
    }
    offset = *(uint64_t *)(ram_start + 0x10);
    printf("kernel memory allocated at 0x%lx.\n", offset);

    /* then we obtain the physically contiguous kernel memory allocated by our driver */
    mem = openKernelMemory();

    ram_start[0x8] = '-';// '-' stands for client has ready to receive new event
    ram_start[0xa] = '0';// '0' stands for qemu ready to receive notify
    
    mem[1] = '0';// 0 stands for qemu has received data completely

    /* Init all */
    pq = InitQueue();
    head = Creat_Node();
    head_new = Creat_Node();

    pthread_mutex_init(&mutex_fd, NULL);
    pthread_mutex_init(&mutex_link, NULL);
    pthread_mutex_init(&mutex_queue, NULL);
    pthread_mutex_init(&mutex_closenotify, NULL);
    
    /* thread for receiving command */
    pthread_create(&thread_cmd, NULL, &recv_cmd, NULL);
    /* thread for receiving event */
    pthread_create(&thread_recv_event, NULL, &recv_events, NULL);
    /* thread for acting event */
    pthread_create(&thread_emulate_event, NULL, &emulate_event, NULL);


    arg_struct *arg;
    Node *p;

    XSetErrorHandler(bad_window_handler);
    int old, new;
    old = new = 0;

    for (;;) {
        enum_new_windows(dpy);

        new = Count_Node(head);
        if (new != old) {
            old = new;
            Print_Node(head);
            //Print_Node(head_new);
        }

        p = head_new;
        while ((p = p->next) != NULL) {
            if (ErrorFlag) {
                pthread_mutex_lock(&mutex_link);
                deleteItem(head, FindByValue(head, p->wid));
                pthread_mutex_unlock(&mutex_link);
                ErrorFlag = 0;
            } else {
                arg = malloc(sizeof(arg_struct));
                arg->dpy = XOpenDisplay(":0.0");
                arg->wid = p->wid;
                pthread_create(&thread_window, NULL, &print_window, (void *)arg);
                //printf("New Guest Window ID: 0x%lx\n", arg->wid);
                printf("[Thread 0x%lx for Win 0x%lx inited]\n", thread_window, arg->wid);
            }
        }
        FlushLink(head_new);
    }
    
    DestoryLink(head);    
    DestoryLink(head_new);
    XCloseDisplay(dpy);
    return 0;
}

