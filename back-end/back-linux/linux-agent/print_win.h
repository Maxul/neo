#ifndef CLIENT_GETIMAGE_H_
#define CLIENT_GETIMAGE_H_

#include <X11/Xlib.h>

/* for 4MB kernel memory */
struct window_image
{
    int64_t win;
    int64_t width, height;
    int64_t size, depth;
    int64_t dx, dy, win_related;

    int64_t title_len, encode, format, nitems;
    int64_t title[30];

    int64_t override;
    int64_t image;
};

typedef struct {
    Display *dpy;
    Window wid;
} arg_struct;

/* thread for print guest window image */
void* print_window(void* arg);

#endif
