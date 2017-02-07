#ifndef SERVER_EVENT_H_
#define SERVER_EVENT_H_

#include <X11/Xlib.h>

struct event_arg
{
    Display *dpy;
    Window wnd;
};

/* add Xevent to queue*/
void* input_event_queue(void* arg);

#endif
