#include "getimage.h"
#include "win_pthreads.h"

#include "link.h"
#include "comu.h"

#include <stdlib.h>
#include <string.h>
#include <malloc.h>

extern char *mem;
extern int offset;
extern unsigned int sharedsize;
extern Node *head;
extern Window active_win;
extern pthread_mutex_t mutex_mem;
extern pthread_mutex_t mutex_actwin;
extern pthread_mutex_t mutex_link;
extern pthread_mutex_t mutex_closenotify;

static char downIMG[4 * 1024 * 1024];

/*thread for get local image*/
void* get_image(void* arg)
{
	int lx = 0, ly = 0;
	int lx_relate, ly_relate;
	int first_image = 1;

	/* height and width for the new window.*/
	unsigned int width, height;
	Window win = ((arg_struct *)arg)->wid;

	char override;
	Window win_relate;

	// 无标题的窗口为二类窗口
	char szWindowText[256];
	szWindowText[0] = 0;
	GetWindowTextA(win, szWindowText, sizeof(szWindowText));
	RECT rectWindow, rectRelateWindow;

	/*get the WIN attributes*/
	GetWindowRect(win, &rectWindow);
	if (szWindowText[0] == 0) {
		override = '1';
		Window junkwin;

		pthread_mutex_lock(&mutex_actwin);
		win_relate = active_win;
		pthread_mutex_unlock(&mutex_actwin);
		GetWindowRect(win_relate, &rectRelateWindow);

		// 获取其相对母窗口的相对位置
		lx = rectRelateWindow.left - rectWindow.left;
		ly = rectRelateWindow.top - rectWindow.top;
	}
	else {
		override = '0';
	}

	/* make the size of new window same as orginal . */
	HBITMAP hBitmap;
	// 显示器屏幕DC
	HDC hScreenDC = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
	HDC hmemDC = CreateCompatibleDC(hScreenDC);
	// 保存位图数据
	PVOID lpvpxldata;
	// 截屏获取的长宽及起点
	INT ixStart, iyStart, iX, iY;
	DWORD dwBitmapArraySize;
	// 位图信息头
	BITMAPINFO bmInfo;
	
	LPRECT lpRect = &rectWindow;
	for (;;)
	{
	LABEL1:
		if (!IsWindow(win))
		{
			pthread_mutex_lock(&mutex_link);
			deleteItem(head, FindByValue(head, win), 0);
			send_notify(win);
			pthread_mutex_unlock(&mutex_link);
			pthread_detach(pthread_self());
			free(arg); // free arg
			return NULL;
		}
		
		pthread_mutex_lock(&mutex_mem);
		/*wait for qemu ready*/
		
		for (;;)
		{
			//printf("now wait for qemu ready! %c(0)\n",*(char *)(mem+sizeof(char)));
			if (*(char *)(mem + sizeof(char)) == '0') //check whether qemu has been ready to receive data
				break;
		}
		
		ixStart = lpRect->left;
		iyStart = lpRect->top;
		iX = lpRect->right - lpRect->left;
		iY = lpRect->bottom - lpRect->top;
		
		if (iX <= 0 || iY <= 0)
		{
			ReleaseDC(0, hScreenDC);
			DeleteDC(hmemDC);
			goto LABEL1;
		}
		
		// 创建BTIMAP
		hBitmap = CreateCompatibleBitmap(hScreenDC, iX, iY);
		// 将BITMAP选择入内存DC。
		SelectObject(hmemDC, hBitmap);

		if (override == '0') {
			TITLEBARINFO tbInfo;
			tbInfo.cbSize = sizeof(TITLEBARINFO);
			GetTitleBarInfo(win, &tbInfo);

			// 不存在标题栏则打印客户区
			if (tbInfo.rcTitleBar.bottom < 0 || tbInfo.rcTitleBar.top < 0) {
				PrintWindow(win, hmemDC, PW_CLIENTONLY);
			}

			else {
				// 打印整个窗口
				PrintWindow(win, hmemDC, 0);

				WINDOWINFO wInfo;
				wInfo.cbSize = sizeof(WINDOWINFO);
				GetWindowInfo(win, &wInfo);

				int titleBarHeight = tbInfo.rcTitleBar.bottom - tbInfo.rcTitleBar.top + wInfo.cyWindowBorders;

				BitBlt(hmemDC, 0, 0, iX, iY, hmemDC, wInfo.cxWindowBorders, titleBarHeight, SRCCOPY);

				iX -= wInfo.cxWindowBorders * 2;
				iY -= titleBarHeight + wInfo.cyWindowBorders;
			}
		}
		else {
			// BitBlt屏幕DC到内存DC，根据所需截取的获取设置参数
			BitBlt(hmemDC, 0, 0, iX, iY, hScreenDC, ixStart, iyStart, SRCCOPY);
		}


		// 添充 BITMAPINFO 结构
		ZeroMemory(&bmInfo, sizeof(BITMAPINFO));
		bmInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmInfo.bmiHeader.biWidth = iX;
		bmInfo.bmiHeader.biHeight = iY;
		bmInfo.bmiHeader.biPlanes = 1;
		bmInfo.bmiHeader.biBitCount = 32;
		bmInfo.bmiHeader.biCompression = BI_RGB;

		dwBitmapArraySize =
			((((bmInfo.bmiHeader.biWidth * bmInfo.bmiHeader.biBitCount) + 31) & ~31) >> 3)
			* bmInfo.bmiHeader.biHeight;
			
		/*prepare image date to send*/
		{
			*((int *)(mem + sizeof(char) * 2 + sizeof(int) * 3)) = iX;
			*((int *)(mem + sizeof(char) * 2 + sizeof(int) * 4)) = iY;

			*((int *)(mem + sizeof(char) * 2 + sizeof(int) * 5)) = dwBitmapArraySize;

			*((char *)(mem + sizeof(char) * 2 + sizeof(int) * 7)) = override;

			*((int *)(mem + sizeof(char) * 3 + sizeof(int) * 8)) = lx;
			*((int *)(mem + sizeof(char) * 3 + sizeof(int) * 9)) = ly;

			if (first_image == 1)
			{
				*(char *)(mem + sizeof(char) * 2 + sizeof(int) * 6 + 190) = '1';
				first_image = 0;
			}
			else
			{
				*(char *)(mem + sizeof(char) * 2 + sizeof(int) * 6 + 190) = '0';
			}

			/*window title*/
			*((int *)(mem + sizeof(char) * 3 + sizeof(int) * 14)) = strlen(szWindowText); //title string len
			*((int *)(mem + sizeof(char) * 3 + sizeof(int) * 15)) = (int)430; //title property encoding
			*((int *)(mem + sizeof(char) * 3 + sizeof(int) * 16)) = (int)8; //title property format
			*((int *)(mem + sizeof(char) * 3 + sizeof(int) * 17)) = (int)strlen(szWindowText); //title property nitems
			memcpy((void *)(mem + sizeof(char) * 3 + sizeof(int) * 18), (void *)(szWindowText), strlen(szWindowText));

			// 获取DIB用于写入到文件
			*(int *)(mem + sizeof(char) * 2 + sizeof(int) * 6 + 200) = 32; // depth
			GetDIBits(hmemDC, hBitmap, 0, iY, downIMG, &bmInfo, DIB_RGB_COLORS);
			
			lpvpxldata = (void *)(mem + sizeof(char) * 2 + sizeof(int) * 6 + 300);
			int scan_line_width = bmInfo.bmiHeader.biBitCount / 8 * iX;
			int pos;
			char *src, *dest;
			for (pos = 0; pos < iY; ++pos)
			{
				src = (char *)downIMG + scan_line_width * pos;
				dest = (char *)lpvpxldata + scan_line_width * (iY - pos - 1);
				RtlCopyMemory(dest, src, scan_line_width);
			}

		}

		*((int *)(mem + sizeof(char) * 2 + sizeof(int) * 6)) = (int)win;
		*((int *)(mem + sizeof(char) * 3 + sizeof(int) * 10)) = (int)win_relate;
		*((char *)(mem + sizeof(char))) = '1';
		pthread_mutex_unlock(&mutex_mem);

	}
	ReleaseDC(0, hScreenDC);
	DeleteDC(hmemDC);
}
