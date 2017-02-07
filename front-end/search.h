#ifndef SERVER_SEARCH_H_
#define SERVER_SEARCH_H_

#include <regex.h>
#include <X11/Xlib.h>

void get_window(Display *dpy, Window window, const char *winname);

#endif
