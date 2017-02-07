#include "actevent.h"
#include "queue.h"
#include <string.h>
#include "win_pthreads.h"

extern Queue *pq;
extern pthread_mutex_t mutex_queue;

// 窗口移动
void  window_move(Window wid) // 传入窗口id
{
	UNREFERENCED_PARAMETER(wid);
}

// 移动鼠标
// x,y 为相对窗口位置
void pointer_move(int dx, int dy, Window window, int button)
{
	// 激活此窗口
	SwitchToThisWindow(window, TRUE);

	// 移到实际窗口相应位置 
	WINDOWINFO wInfo;
	GetWindowInfo(window, &wInfo);
	SetCursorPos(wInfo.rcWindow.left + dx, wInfo.rcWindow.top + dy);
}

int mouse_click(int x, int y, Window window, int button, int is_press) {

	INPUT Input = { 0 };
	ZeroMemory(&Input, sizeof(INPUT));

	Input.type = INPUT_MOUSE;

	if (is_press) {
		Input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
	}
	else {
		Input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
	}

	SendInput(1, &Input, sizeof(INPUT));

	return -1;
}

//设置模拟键盘输入 
int key_press(int x, int y, unsigned int keycode, int is_press) {
	INPUT Input = { 0 };
	ZeroMemory(&Input, sizeof(INPUT));

	Input.type = INPUT_KEYBOARD;
	Input.ki.wVk = keycode;

	// 释放按键，这非常重要 
	if (!is_press) {
		Input.ki.dwFlags = KEYEVENTF_KEYUP;
	}

	SendInput(1, &Input, sizeof(INPUT));

	return -1;
}

// 激活窗口
int window_activate(Window wid) {
	SwitchToThisWindow(wid, TRUE);
	return 0;
}


void window_resize(Window win, int width, int height)
{
	SetWindowPos(win, HWND_TOPMOST, 0, 0, width, height, SWP_NOMOVE);
}

/*thread for Receive event from server*/
void* act_event(void* arg)
{
	Window win;
	unsigned int keycode;
	Item item_tmp;
	int x, y, button;

	for (;;)
	{
		pthread_mutex_lock(&mutex_queue);

		if (GetSize(pq) != 0)
		{
			//printf("have a new event from queue!\n");
			DeQueue(pq, &item_tmp);
			pthread_mutex_unlock(&mutex_queue);
			switch (item_tmp.type)
			{
			case('0') ://button press
			{
				x = item_tmp.x;
				y = item_tmp.y;
				button = item_tmp.button;
				win = item_tmp.source_wid;
				pointer_move(x, y, win, button);
				//printf("send move x:%d,y:%d\n",x,y);
				mouse_click(x, y, win, button, 1);
				//printf("button press event x=%d,y=%d,button=%d\n",x,y,button);
			}
					   break;
			case('1') ://pointer move
			{
				x = item_tmp.x;
				y = item_tmp.y;
				button = item_tmp.button;
				win = item_tmp.source_wid;
				pointer_move(x, y, win, button);
				//printf("pointer move event movie x=%d,y=%d\n",x,y);
			}
					   break;
			case('2') ://button release
			{
				x = item_tmp.x;
				y = item_tmp.y;
				button = item_tmp.button;
				win = item_tmp.source_wid;
				mouse_click(x, y, win, button, 0);
				//printf("button release event x=%d,y=%d,button=%d\n",x,y,button);
			}
					   break;
			case('3') ://key press
			{
				x = item_tmp.x;
				y = item_tmp.y;
				keycode = item_tmp.button;
				win = item_tmp.source_wid;
				key_press(x, y, keycode, 1);
				//printf("key press event x=%d,y=%d,button=%d\n",x,y,button);
			}
					   break;
			case('4') ://key release
			{
				x = item_tmp.x;
				y = item_tmp.y;
				keycode = item_tmp.button;
				win = item_tmp.source_wid;
				key_press(x, y, keycode, 0);
				//printf("key release event x=%d,y=%d,button=%d\n",x,y,button);
			}
					   break;
			case('5') ://window resize
			{
				x = item_tmp.x;
				y = item_tmp.y;
				keycode = item_tmp.button;
				win = item_tmp.source_wid;
				window_resize(win, x, y);
				//printf("act window resize event width=%d,height=%d\n",x,y);
			}
					   break;

			case('6') ://window close
			{
				x = item_tmp.x;
				y = item_tmp.y;
				keycode = item_tmp.button;
				win = item_tmp.source_wid;
				window_activate(win);
				CloseWindow(win);
				//printf("act window close!\n");
			}
					   break;

			case('7') ://window expose
			{
				win = item_tmp.source_wid;
				window_move(win);
				//printf("act window resize event width=%d,height=%d\n",x,y);
			}
					   break;

			default:
			{
				//printf("default discard!\n");
			}
			break;
			}
		}
		else
		{
			pthread_mutex_unlock(&mutex_queue);
		}
		Sleep(10);
	}
}
