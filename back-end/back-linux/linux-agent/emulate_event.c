#include "emulate_event.h"
#include "queue.h"
#include "list.h"

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

extern Node *head;
extern pthread_mutex_t mutex_queue;
extern Queue *pq;
extern Display *dpy;

/* make specified window to be foreground */
/* using mouse click to activate it */
static void window_activate(Window wid, Display *dpy)
{
    XEvent xev;
    XWindowAttributes wattr;

    memset(&xev, 0, sizeof(xev));
    xev.type = ClientMessage;
    xev.xclient.display = dpy;
    xev.xclient.window = wid;
    xev.xclient.message_type = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = 2L; /* 2 == Message from a window pager */
    xev.xclient.data.l[1] = CurrentTime;

    /* we do not have to check the return value,
    since user could move more to foregound the window,
    it actalliy work here, please work it out */
    XGetWindowAttributes(dpy, wid, &wattr);
    XSendEvent(dpy, wattr.screen->root, False,
               SubstructureNotifyMask | SubstructureRedirectMask,
               &xev);
}

static void pointer_move(int x, int y, Window window, Display *dpy)
{
    /* to fix override window position problem */
    window_activate(window, dpy);
    XWarpPointer(dpy, None, window, 0, 0, 0, 0, x, y);
    XFlush(dpy);
}

static void mouse_click(int x, int y, Window window, int button, int is_press, Display *dpy)
{
    window_activate(window, dpy);
    XTestFakeButtonEvent(dpy, button, is_press, CurrentTime);
    XFlush(dpy);
}

static void key_press(int x, int y, unsigned int keycode, int is_press, Display *dpy)
{
    XTestFakeKeyEvent(dpy, keycode, is_press, CurrentTime);
    XFlush(dpy);
}

/* FIXME: Performance Issue */
static void window_resize(Window win, int width, int height, Display *dpy)
{
    XResizeWindow(dpy, win, width, height);
    XFlush(dpy);
}

/* thread for receiving events from server */
void* emulate_event(void* unused)
{
    Window win;
    Item item_tmp;

    int x, y, button;
    unsigned int keycode;

    int i;

    for (;;) {

        if (GetSize(pq) != 0) {
            //printf("have a new event from queue!\n");
            /* 之前查看队列时也用互斥，我想可以不用，
            修改队列时再用，全局只有这里 DeQueue 操作，发现不可以！*/
            pthread_mutex_lock(&mutex_queue);
            DeQueue(pq, &item_tmp);
            pthread_mutex_unlock(&mutex_queue);

            win = item_tmp.source_wid;
            /* MUST: make sure it's still alive */
            if (-1 == FindByValue(head, win))
                continue;

            x = item_tmp.x;
            y = item_tmp.y;
            keycode = button = item_tmp.button;

            switch(item_tmp.type) {
            case('0')://button press
                pointer_move(x, y, win, dpy);
                mouse_click(x, y, win, button, 1, dpy);
                //printf("button press event x=%d,y=%d,button=%d\n",x,y,button);
                break;

            case('1')://pointer move
                pointer_move(x, y, win, dpy);
                //printf("pointer move event win=%lx x=%d,y=%d\n",win, x,y);
                break;

            case('2')://button release
                mouse_click(x, y, win, button, 0, dpy);
                //printf("button release event x=%d,y=%d,button=%d\n",x,y,button);
                break;

            case('3')://key press
                key_press(x, y, keycode, 1, dpy);
                //printf("key press event x=%d,y=%d,button=%d\n",x,y,button);
                break;

            case('4')://key release
                key_press(x, y, keycode, 0, dpy);
                //printf("key release event x=%d,y=%d,button=%d\n",x,y,button);
                break;

            case('5')://window resize
                window_resize(win, x, y, dpy);
                //printf("act window resize event width=%d,height=%d\n",x,y);
                break;

            case('6')://window close
                i = 10; /* make sure the targeted window is activated */
                while (i--)
                    window_activate(win, dpy);
                /* ALT + F4 */
                key_press(x, y, 0x40, 1, dpy);
                key_press(x, y, 0x46, 1, dpy);
                key_press(x, y, 0x46, 0, dpy);
                key_press(x, y, 0x40, 0, dpy);
                //printf("act window close on %lx!\n", win);

                break;
            case('7')://window expose
                //window_move(dpy, win);
                //printf("act window resize event width=%d,height=%d\n",x,y);
                break;

            default:
                //printf("default discard!\n");
                break;

            }
        }
    }
}

