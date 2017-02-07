#ifndef CLIENT_WIN_H_
#define CLIENT_WIN_H_

#include <X11/Xlib.h>

void enum_new_windows(Display *disp);

void get_relate_window(Display *dpy, Window *window_ret);

#endif
