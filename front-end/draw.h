#ifndef SERVER_DRAW_H_
#define SERVER_DRAW_H_

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>
#include <pthread.h>
#include <malloc.h>
#include <unistd.h>
#include <memory.h>
#include <semaphore.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xcomposite.h>

#define SHARED_MEM_SIZE (1024*1024*5)

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

#define imem ((struct window_image *)(mem + 2))

typedef struct
{
    Window wid;
    char override;
} draw_arg;

/*thread for draw picture*/
void* draw_window(void* arg);

#endif
