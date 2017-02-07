#include "activewin.h"
#include "win_pthreads.h"

Window active_win;
extern pthread_mutex_t mutex_actwin;

/*thread for get current active window* in order to get related window ID*/
void* get_active_win(void* arg)
{
	for (;;)
	{
		pthread_mutex_lock(&mutex_actwin);
		active_win = GetForegroundWindow();
		//printf("%x\n", active_win);
		pthread_mutex_unlock(&mutex_actwin);
		Sleep(100);
	}
}
