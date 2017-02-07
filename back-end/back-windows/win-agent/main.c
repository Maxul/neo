#include "stdafx.h"
#include "winio.h"
#include "win_pthreads.h"
#include "queue.h"
#include "link.h"
#include "comu.h"
#include "actevent.h"
#include "activewin.h"
#include "listwin.h"
#include "filexchange.h"
#include "getimage.h"

CHAR szWinIoDriverPath[MAX_PATH];
BOOL g_Is64BitOS;
HANDLE hDriver = INVALID_HANDLE_VALUE;
BOOL IsWinIoInitialized = FALSE;

int fd;
char *mem_start; // 虚拟机物理内存首地址
char *mem;		// 分配连续物理内存首地址
int offset;
int ErrorFlag = 0;
unsigned int sharedsize = 5 * 1024 * 1024;

Queue *pq;
Node *head, *head_new;

pthread_mutex_t mutex_mem;
pthread_mutex_t mutex_queue;
pthread_mutex_t mutex_link;
pthread_mutex_t mutex_actwin;
pthread_mutex_t mutex_closenotify;
pthread_mutex_t mutex_fd;

tagPhysStruct Phys;

PCHAR _stdcall OpenContiguousSection(tagPhysStruct *PhysStruct)
{
	return MapPhysToLin(PhysStruct);
}

BOOL _stdcall CloseContiguousSection(tagPhysStruct *PhysStruct)
{
	return UnmapPhysicalMemory(PhysStruct);
}

// 得到一连续物理地址内容
DWORD _stdcall GetOffset()
{
	DWORD PhysAddr; // 内核分配物理地址

	if (!IsWinIoInitialized)
		return FALSE;

	GetPhysLong((PBYTE)0x2, &PhysAddr);
	
	return PhysAddr;
}

/*---------------------------main()----------------------------- */
VOID __cdecl main(_In_ ULONG argc, _In_reads_(argc) PCHAR argv[])
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);

	//ShutdownWinIo();
	printf("now wait...\n");
	printf("now start ...\n");
	InitializeWinIo();

	if (!IsWinIoInitialized)
	{
		int i;
		for (;;)
		{
			DWORD PhysAddr64;
			DWORD Length;
			scanf_s("%x", &PhysAddr64, sizeof(PhysAddr64));
			scanf_s("%x", &Length, sizeof(Length));

			if (Length == 0 || Length == 1)
			{
				ShutdownWinIo();
				printf("REMOVED.\n");
				return;
			}
			scanf_s("%d", &i, sizeof(i));
			if (1 == i)
				GetContiguousPhysLong(PhysAddr64, &Length);
			else
				SetContiguousPhysLong(PhysAddr64, (DWORD)0xff, &Length);
		}
		
	}

	
	// 休息一下
	Sleep(10);

	pthread_t thread_event_id1,
		thread_event_id2,
		thread_event_id3,
		thread_event_active,
		thread_filexchange;
	
	pq = InitQueue();

	// 互斥锁初始化
	pthread_mutex_init(&mutex_mem, NULL);
	pthread_mutex_init(&mutex_queue, NULL);
	pthread_mutex_init(&mutex_link, NULL);
	pthread_mutex_init(&mutex_actwin, NULL);
	pthread_mutex_init(&mutex_closenotify, NULL);
	pthread_mutex_init(&mutex_fd, NULL);

	Node *p;
	int print_count = 0;

	head = Creat_Node();
	head_new = Creat_Node();
	
	Phys.pvPhysAddress = (DWORD64)(DWORD32)0x0;
	Phys.dwPhysMemSizeInBytes = 32;
	// mem 为虚拟机物理内存首地址
	mem = OpenContiguousSection(&Phys);
	
	printf("Waiting for shared memory allocate.");
	
	while (*mem != '#' || *(mem + 1) != '*')
	{
		
		print_count++;
		if (print_count % 1000000 == 0)
		{
			printf(" .");
			print_count = 0;
		}
	}
	
	// 获取连续内核内存偏移量
	offset = GetOffset();
	printf("\nshared memory allocated at 0X%0X.\n", offset);
	// mem_start 为虚拟机物理内存首地址
	mem_start = mem;
	
	//init shared memory
	Phys.pvPhysAddress = offset;
	Phys.dwPhysMemSizeInBytes = sharedsize;
	
	mem = OpenContiguousSection(&Phys);
	memset(mem, 0, sharedsize);

	// 0 stands for qemu has received data completely
	*((char *)(mem + sizeof(char))) = '0';
	mem_start[8] = '-'; //- stands for client has ready to receive new event
	mem_start[10] = '0'; //'0' stands for qemu ready to receive notify
	
	//thread for receive event
	pthread_create(&thread_event_id2, NULL, &receive_queue, NULL);
	//thread for act event
	pthread_create(&thread_event_id3, NULL, &act_event, NULL);
	//thread for get current active window in order to get related window ID
	pthread_create(&thread_event_active, NULL, &get_active_win, NULL);
	//thread for listen file exchange
	//pthread_create(&thread_filexchange, NULL, &file_listen, NULL);
	
	arg_struct *arg;
	char cs;
	char buf[65535];
	
	// 主线程
	for (;;)
	{
		//scan
		// 遍历所有可视窗口
		list_view_windows();
		list_override_window();
		
		// 获取窗口截图
		// 只针对client启动后的新窗口
		p = head_new;
		while ((p = p->next) != NULL)
		{
			ErrorFlag = !IsWindow(p->wid);
			if (ErrorFlag)  //this windows is invalid
			{
				pthread_mutex_lock(&mutex_link);
				deleteItem(head, FindByValue(head, p->wid), 1);
				ErrorFlag = 0;
				pthread_mutex_unlock(&mutex_link);
			}
			else
			{
				arg = (arg_struct *)malloc(sizeof(arg_struct));
				arg->wid = p->wid;
				printf("create thread2 status:%d\n", pthread_create(&thread_event_id1, NULL, &get_image, (void *)arg));
				// printf("start new thread for 0x%x\n",arg->wid);
			}
		}
		//flush head_new
		FlushLink(head_new);

		//set checked = 0
		p = head;
		while ((p = p->next) != NULL) {
			p->checked = 0;
		}
		Sleep(500);
		
	}
}
