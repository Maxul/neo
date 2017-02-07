#include "comu.h"
#include "win_pthreads.h"
#include "queue.h"

extern char *mem_start; // 虚拟机物理内存首地址
extern char *mem;		// 分配连续物理内存首地址
extern Queue *pq;
extern pthread_mutex_t mutex_queue;
extern pthread_mutex_t mutex_closenotify;
extern pthread_mutex_t mutex_fd;

/*receive event to queue*/
// 队列是用来接收事件流的
// 以下是一个线程
void *receive_queue(void *arg)
{
	Item item_tmp;
	char c;

	for (;;)
	{
		pthread_mutex_lock(&mutex_fd);//P

		c = mem_start[8];

		pthread_mutex_unlock(&mutex_fd);//V

		if (c != '-') // not '-' stands for obtaining new event
		{
			item_tmp.x = *((int *)(mem + sizeof(char) * 2));
			item_tmp.y = *((int *)(mem + sizeof(char) * 2 + sizeof(int) * 1));

			item_tmp.button = *((unsigned int *)(mem + sizeof(char) * 2 + sizeof(int) * 2));

			item_tmp.type = c;

			item_tmp.source_wid = (Window)*((int *)(mem + sizeof(char) * 3 + sizeof(int) * 7));

			pthread_mutex_lock(&mutex_queue);
			EnQueue(pq, item_tmp);
			pthread_mutex_unlock(&mutex_queue);

			c = '-';

			pthread_mutex_lock(&mutex_fd);//P
			mem_start[8] = c; //write to VPA /dev/mem[7] ???
			pthread_mutex_unlock(&mutex_fd);//V
			//printf("receive event from qemu:x:%d,y:%d,button:%d,type:%d,window:0x%x\n",item_tmp.x,item_tmp.y,item_tmp.button,item_tmp.type,item_tmp.soure_wid);
		}
		Sleep(20);
	}
}

/*send close notify to qemu-kvm*/
void send_notify(Window win)
{
	char c;
	int wid = (int)win;

	pthread_mutex_lock(&mutex_closenotify);

	for (;;)
	{
		pthread_mutex_lock(&mutex_fd);//P
		c = mem_start[10];
		pthread_mutex_unlock(&mutex_fd);//V

		if (c == '0')//qemu ready to receive
		{
			pthread_mutex_lock(&mutex_fd);//P
			*(int *)(mem_start + 11) = wid;
			c = '1'; //'1' stands for client is sendding notify
			mem_start[10] = c;
			pthread_mutex_unlock(&mutex_fd);//V
			pthread_mutex_unlock(&mutex_closenotify);
			pthread_detach(pthread_self());
			return;
		}
	}
}
