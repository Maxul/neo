#include "manipulate_win.h"
#include "print_win.h"
#include "list.h"
#include "com.h"

#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <pthread.h>
#include <sys/mman.h>
#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <X11/extensions/Xdamage.h>

extern char *mem;

extern Node *head;

extern pthread_mutex_t mutex_mem;
extern pthread_mutex_t mutex_link;
extern pthread_mutex_t mutex_closenotify;

/* 代码实在有点多，所以还是单独列一个文件 */

#define destroy_self()                          \
{                                               \
    pthread_mutex_lock(&mutex_link);            \
    deleteItem(head, FindByValue(head, win));   \
    send_notify(win);                           \
    pthread_mutex_unlock(&mutex_link);          \
                                                \
    XCloseDisplay(dpy);                         \
    free(arg);                                  \
                                                \
    pthread_detach(pthread_self());             \
    return NULL;                                \
}

/*thread for get local image*/
void* print_window(void* arg)
{
    XImage *image;
    XWindowAttributes window_attributes_return, window_attributes_return_relate;

    /* height and width for this new window. */
    unsigned int width, height;

    char override;

    Window win_relate;

    XTextProperty win_name_ret;

    int event_base, error_base; // for XDamage

    Display *dpy = ((arg_struct *)arg)->dpy;
    Window win = ((arg_struct *)arg)->wid;

    XSelectInput(dpy, win, StructureNotifyMask); // for window close event
    XDamageQueryExtension(dpy, &event_base, &error_base); // for XDamage
    XDamageCreate(dpy, win, XDamageReportRawRectangles);

    int dx = 0, dy = 0;
    int lx_relate, ly_relate;

    /* FIXME: sometimes we don't have to do it again */
    XGetWindowAttributes(dpy, win, &window_attributes_return);
    if (1 == window_attributes_return.override_redirect) {

        Window junkwin;

        get_relate_window(dpy, &win_relate);

        XGetWindowAttributes(dpy, win_relate, &window_attributes_return_relate);

        /*******************************************************************************
        Translating Screen Coordinates
        Applications sometimes need to perform a coordinate transformation from the co-
        ordinate space of one window to another window or need to determine which win-
        dow the pointing device is in. XTranslateCoordinates and XQueryPointer fulfill
        these needs (and avoid any race conditions) by asking the X server to perform these
        operations.
        To translate a coordinate in one window to the coordinate space of another window,
        use XTranslateCoordinates.
        MUST: You must figure out what this is for, and how in deed!!!
        *******************************************************************************/
        XTranslateCoordinates(dpy, win_relate,
                              window_attributes_return_relate.root,
                              -window_attributes_return_relate.border_width,
                              -window_attributes_return_relate.border_width,
                              &lx_relate, &ly_relate, &junkwin);

        dx = window_attributes_return.x - lx_relate;
        dy = window_attributes_return.y - ly_relate;

        override = '1';
    } else {
        override = '0';
    }

    /* make the size of new window same as orginal */
    XEvent guest_event;
    int first = 1;
    for (;;) {
        if (first == 1) {
            first = 0;
            goto l1;
        }

        XNextEvent(dpy, &guest_event);

        switch(guest_event.type) {

        /* window close event */
        case UnmapNotify:
        case DestroyNotify:
            destroy_self();
            break; /* will NEVER reach here */

        /* XDamage notify */
        default:
            if (guest_event.type == event_base + XDamageNotify) {
l1:
                XGetWindowAttributes(dpy, win, &window_attributes_return);
                width = window_attributes_return.width;
                height = window_attributes_return.height;

                /* can NO MORE get image */
                if ((image = XGetImage(dpy, win, 0, 0, width, height, AllPlanes, ZPixmap)) == NULL) {
                    perror("XGetImage error");
                    destroy_self();
                }
#define old
                /* get window title */
                XGetWMName(dpy, win, &win_name_ret);
#ifdef new
                static struct window_image wi;
                wi.win = win;
                wi.win_related = win_relate;
                wi.override = override;
                wi.width = width;
                wi.height = height;
                wi.size = malloc_usable_size((void *)(image->data));
                wi.depth = image->depth;
                wi.dx = dx;
                wi.dy = dy;
                wi.title_len = malloc_usable_size((void *)win_name_ret.value);
                wi.encode = (int64_t)win_name_ret.encoding;
                wi.format = (int64_t)win_name_ret.format;
                wi.nitems = (int64_t)win_name_ret.nitems;
                memcpy(&(wi.title), win_name_ret.value, wi.title_len);
#endif
                /* copy data to kernel memory */
                pthread_mutex_lock(&mutex_mem);

                /* Let's wait for Qemu to be prrpared */
                for (;;) {
                    if (*(char *)(mem + sizeof(char)) == '0')
                        break;
                }

                /* FIXME: should prepare them already and copy to kernel all at once */
                /* make them virtually contiguous */
#ifdef new
                memcpy((void *)(mem + 2), (void *)&wi, sizeof(wi));
                //printf("win 0x%lx override %c\n", wi.win, wi.override);

                /* comment this you'll get to know what's captured */
                memcpy((void *)(mem + 2 + sizeof(wi)), (void *)(image->data), malloc_usable_size((void *)(image->data)));
#endif

#ifdef old
                *((int *)(mem + 2 + sizeof(int) * 3)) = width;
                *((int *)(mem + 2 + sizeof(int) * 4)) = height;
                *((int *)(mem + 2 + sizeof(int) * 5)) = malloc_usable_size((void *)(image->data));
                *((int *)(mem + 2 + sizeof(int) * 6)) = win;
                *((char *)(mem + 2 + sizeof(int) * 7)) = override;

                /* DEPTH field supported by Maxul Lee <28th Apr 2015> */
                *((int *)(mem + 2 + sizeof(int) * 6 + 200)) = image->depth;

                /* comment this you'll get to know what's captured */
                memcpy((void *)(mem + 2 + sizeof(int) * 6 + 300), (void *)(image->data), malloc_usable_size((void *)(image->data)));

                *((int *)(mem + 3 + sizeof(int) * 8)) = dx;
                *((int *)(mem + 3 + sizeof(int) * 9)) = dy;

                *((int *)(mem + 3 + sizeof(int) * 10)) = win_relate;

                /* window title */
                *((int *)(mem + 3 + sizeof(int) * 14)) = malloc_usable_size((void *)win_name_ret.value); //title string len
                *((int *)(mem + 3 + sizeof(int) * 15)) = (int)win_name_ret.encoding; //title property encoding
                *((int *)(mem + 3 + sizeof(int) * 16)) = (int)win_name_ret.format; //title property format
                *((int *)(mem + 3 + sizeof(int) * 17)) = (int)win_name_ret.nitems; //title property nitems
                memcpy((void *)(mem + 3 + sizeof(int) * 18), (void *)(win_name_ret.value), malloc_usable_size((void *)win_name_ret.value));
#endif
                /* tell outside we're done */
                *((char *)(mem + 1)) = '1';
                pthread_mutex_unlock(&mutex_mem);

                /* we must free the image in order not to leak */
                XDestroyImage(image);
                XFree(win_name_ret.value);
            }
            break;
        }
    }
}

