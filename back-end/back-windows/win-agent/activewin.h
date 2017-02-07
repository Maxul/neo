#include "stdafx.h"

int get_active_window(Window *window_ret);

/*thread for get current active window* in order to get related window ID*/
void* get_active_win(void* arg);