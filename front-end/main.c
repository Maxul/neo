#include "list.h"
#include "queue.h"

#include "ipc.h"

#include "draw.h"
#include "event.h"
#include "search.h"

#include <fcntl.h>
#include <memory.h>

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <pthread.h>

#include <sys/shm.h>
#include <semaphore.h>

#include <sys/ipc.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>

#define SHARED_MEM_SIZE (1024*1024*5)
#define BORDER 3 //color border

/* shared memory with qemu */
char *mem;
/* mutex that synchronises for shm */
sem_t *mutex;

/* convenience for XGetTextProperty to obtain the WM_NAME property */
Window desktop;
Display *dpy = NULL;

int color_border = BORDER;
unsigned int border_color;

/* head that links all re-directed window nodes together */
Node *head;
Queue *pq;
Queue *pq_close;

pthread_mutex_t mutex_queue;
pthread_mutex_t mutex_close_queue;
pthread_mutex_t mutex_link;

XSetWindowAttributes xs;

static int sem_id_close; /* FIXME: no need to be static */

void connect_to_guest(char *auth)
{
    int shm_id;

    key_t key_close; // ==> sem_id_close
    key_t key; // ==> sem_id

    /* init all ipcs */
    key_close = ftok(auth, 0x03);
    key = ftok(auth, 0xf);
    if (key_close == -1 || key == -1)
    {
        perror("ftok error");
        exit(1);
    }

    sem_id_close = semget(key_close, 1, IPC_CREAT | 0600);
    sem_id = semget(key, 1, IPC_CREAT | 0600);
    if (sem_id_close == -1 || sem_id == -1)
    {
        perror("semget error");
        exit(1);
    }

    shm_id = shmget(key, SHARED_MEM_SIZE, IPC_CREAT | 0600);
    if (shm_id == -1)
    {
        perror("shmget error");
        exit(1);
    }
    printf("VM: %s\tsem: %d\n", auth, sem_id);

    mem = (char *)shmat(shm_id, NULL, 0);
    mem[0] = '-';
    *((char *)(mem + 1024 * 1024 * 4 + sizeof(int))) = '0';
}

int main(int argc, char** argv)
{
    int wincount = 0;	/* total window */

    Window win_receive;
    pthread_t thread_draw, thread_close_notify;
    draw_arg *arg_tmp;

    Item item_notify;
    char override;
    xs.override_redirect = True;

    border_color = (unsigned int)strtoul(argv[1], 0, 0);

    /* Check Xrender Support */
    int major_opcode, first_event, first_error;
    XInitThreads();
    dpy = XOpenDisplay(NULL);
    if (XQueryExtension(dpy, RENDER_NAME, &major_opcode,
                        &first_event, &first_error) == False)
    {
        fprintf(stderr, "No Xrender-Support\n");
        return 1;
    }

    get_window(dpy, DefaultRootWindow(dpy), "桌面");

    /** @MUST: try connecting with guest agent */
    connect_to_guest(argv[argc - 1]);

    /*init link*/
    head = Creat_Node();

    pq = InitQueue();
    pq_close = InitQueue();

    pthread_mutex_init(&mutex_queue, NULL);
    pthread_mutex_init(&mutex_close_queue, NULL);

    pthread_mutex_init(&mutex_link, NULL);

    int flag_value;

    /* create new thread for close notify event */
    pthread_create(&thread_close_notify, NULL, &close_notify_queue, NULL);

    *((char *)(mem + 1024 * 1024 * 4 + sizeof(int))) = '0';
    for (;;)
    {
        flag_value = semctl(sem_id_close, 0, GETVAL);
        if (flag_value)
            goto exit_loop;

        semaphore_p(sem_id);//P
        pthread_mutex_lock(&mutex_link);
        
        win_receive = *((int *)(mem + 2 + sizeof(int) * 6)); // new guest window
        //printf("%c %lx\n", *(char *)(mem + SHARED_MEM_SIZE - 0xf), win_receive);
        //*(char *)(mem + SHARED_MEM_SIZE - 0xf) = 'Y';

        if (*(char *)(mem + SHARED_MEM_SIZE - 0xf) == 'Y' &&
                FindByValue(head, win_receive) == -1)
        {

            InsertItem(head, 0, win_receive);
            pthread_mutex_unlock(&mutex_link);

            *(char *)(mem + SHARED_MEM_SIZE - 0xf) = 'N';
            wincount++;
            override = *((char *)(mem + 2 + sizeof(int) * 7));

            //if ('0' == override)
            Print_Node(head);
            //printf("InsertItem 0x%lx\n", win_receive);

            arg_tmp = malloc(sizeof(draw_arg));
            arg_tmp->wid = win_receive;
            arg_tmp->override = override;
            
            /* create a new thread to draw */
            pthread_create(&thread_draw, NULL, &draw_window, (void *)arg_tmp);
        }
        else
        {
            pthread_mutex_unlock(&mutex_link);
            semaphore_v(sem_id);//V
        }

        /* check requests in close_queue */
        pthread_mutex_lock(&mutex_close_queue);
        //Print_Queue(pq_close);
        while (GetSize(pq_close) > 0) {
            /* get a close request item from close_queue */
            DeQueue(pq_close, &item_notify);

            pthread_mutex_lock(&mutex_link);
            Node *close_node = FindNodeByValue(head, item_notify.source_wid);
            if (close_node != NULL)
                close_node->close = 1;
            else
                /* 来不及创建新线程，但收到了关闭信号 */
                printf("No Guest WinID 0x%lx, To Be Closed!\n", item_notify.source_wid);
            pthread_mutex_unlock(&mutex_link);
        }
        pthread_mutex_unlock(&mutex_close_queue);
        
    }

exit_loop:
    DestroyQueue(pq_close);
    DestroyQueue(pq);
    DestoryLink(head);
    
    semctl(sem_id_close, 0, IPC_RMID);
    //semctl(sem_id, 0, IPC_RMID);
    XCloseDisplay(dpy);
    printf("AWE Server Exit Success!\n");
    return 0;
}

