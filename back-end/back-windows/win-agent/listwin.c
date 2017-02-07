#include "listwin.h"
#include "link.h"
#include "win_pthreads.h"

extern Node *head, *head_new;
extern pthread_mutex_t mutex_link;

BOOL CALLBACK EnumFunc(HWND hwnd, LPARAM lParam)
{
	DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
	DWORD dwExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
	if (!(dwStyle & WS_VISIBLE) || (dwStyle & WS_CHILD)) {
		return TRUE; // 跳过并继续遍历
	}

	int loc;
	RECT rectWindow;
	/* Only visible windows that are present on the desktop are interesting here */
	if (GetWindowRect(hwnd, &rectWindow)) {
		// 获取标题
		char szWindowText[256];
		szWindowText[0] = 0;
		GetWindowTextA(hwnd, szWindowText, sizeof(szWindowText));
		printf("%s\n", szWindowText);

		if (szWindowText[0] == 0
			&& (
			(dwStyle == (WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS)
			&& dwExStyle == (WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_TOPMOST))
			|| (dwStyle == (WS_POPUP | WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS | WS_CLIPCHILDREN)
			&& dwExStyle == (WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE))
			|| (dwStyle == (WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN)
			&& dwExStyle == (WS_EX_TOOLWINDOW))
			))
		{
			pthread_mutex_lock(&mutex_link);
			if ((loc = FindByValue(head, hwnd)) == -1) {
				InsertItem(head, Count_Node(head), hwnd);//add to head
				InsertItem(head_new, Count_Node(head_new), hwnd);//add to head_new
			}
			/*set checked = 1*/
			else {
				FindNodeByNo(head, loc)->checked = 1;
			}
			pthread_mutex_unlock(&mutex_link);
			return TRUE;
		}

		if (strcmp(szWindowText, "开始")
			&& strcmp(szWindowText, "Program Manager")
			&& szWindowText[0] != 0)
		{
			pthread_mutex_lock(&mutex_link);
			if ((loc = FindByValue(head, hwnd)) == -1) {
				InsertItem(head, Count_Node(head), hwnd);//add to head
				InsertItem(head_new, Count_Node(head_new), hwnd);//add to head_new
			}
			/*set checked = 1*/
			else {
				FindNodeByNo(head, loc)->checked = 1;
			}
			pthread_mutex_unlock(&mutex_link);
		}
	}
	return TRUE; /* continue enumeration */
}


/*list view window*/
void list_view_windows()
{
	HDC param = GetDC(HWND_DESKTOP);

	EnumWindows(EnumFunc, (LPARAM)&param);

	ReleaseDC(HWND_DESKTOP, param);
}

/*list_override_windows*/
void list_override_window()
{
	Print_Node(head);
}
