/*
 * event.c
 * This file is part of VmOs
 *
 * Copyright (C) 2017 - Ecular, Maxul
 *
 * VmOs is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * VmOs is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with VmOs. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ipc.h"
#include "event.h"
#include "list.h"
#include "queue.h"

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#include <X11/Xlib.h>

extern sem_t *mutex;

extern Queue *pq;

extern Node *head;

extern int color_border;

extern pthread_mutex_t mutex_link;
extern pthread_mutex_t mutex_queue;

static void input_event_enqueue(Item item)
{
    pthread_mutex_lock(&mutex_queue);
    EnQueue(pq, item);
    pthread_mutex_unlock(&mutex_queue);
}

void* input_event_queue(void* arg)
{
    XEvent host_event;

    Time last_move_time = 0;

    Item item;

    Display *dpy_wnd = ((struct event_arg *)(arg))->dpy;

    Node *find_result;

    pthread_mutex_lock(&mutex_link);
    item.source_wid = FindWidByValue(head, ((struct event_arg *)(arg))->wnd);
    pthread_mutex_unlock(&mutex_link);

    for (;;)
    {
        XNextEvent(dpy_wnd, &host_event);

        switch (host_event.type)
        {
        case ButtonPress://code 0

            item.x = host_event.xbutton.x - color_border;
            item.y = host_event.xbutton.y - color_border;
            item.button = host_event.xbutton.button;
            item.event_type = '0';

            input_event_enqueue(item);

            break;

        case ButtonRelease://code 2

            item.x = host_event.xbutton.x - color_border;
            item.y = host_event.xbutton.y - color_border;
            item.button = host_event.xbutton.button;
            item.event_type = '2';

            input_event_enqueue(item);

            break;

        case MotionNotify://code 1

            /* 控制传输频率 */
            if (host_event.xbutton.time - last_move_time >= 120) {
                item.x = host_event.xbutton.x;
                item.y = host_event.xbutton.y;
                item.event_type = '1';

                input_event_enqueue(item);

                last_move_time = host_event.xbutton.time;
            }

            break;

        case KeyPress://code 3

            item.x = host_event.xbutton.x - color_border;
            item.y = host_event.xbutton.y - color_border;
            item.button = host_event.xkey.keycode;
            item.event_type = '3';

            input_event_enqueue(item);

            break;

        case KeyRelease://code 4

            item.x = host_event.xbutton.x - color_border;
            item.y = host_event.xbutton.y - color_border;
            item.button = host_event.xkey.keycode;
            item.event_type = '4';

            input_event_enqueue(item);

            break;

        /* windows resize */
        case ConfigureNotify://code 5

            pthread_mutex_lock(&mutex_link);

            find_result = FindNodeBySwinValue(head, host_event.xconfigure.window);
            if (find_result == NULL)
            {
                printf("swid=0x%lx. not find Node!\n", host_event.xconfigure.window);
                pthread_mutex_unlock(&mutex_link);
                goto input_event_exit;
            }

            /* only in this case it's called "RESIZE" */
            if (host_event.xconfigure.width - color_border * 2 != find_result->width
                    || host_event.xconfigure.height - color_border * 2 != find_result->height)
            {

                item.x = host_event.xconfigure.width - color_border * 2;
                item.y = host_event.xconfigure.height - color_border * 2;

                item.event_type = '5';

                input_event_enqueue(item);

                find_result->width = item.x;
                find_result->height = item.y;
                find_result->resize = 1;

            }

            pthread_mutex_unlock(&mutex_link);

            break;

        /* window close */
        case ClientMessage://code 6

            item.x = 0;
            item.y = 0;
            item.button = 0;
            item.event_type = '6';

            input_event_enqueue(item);

            break;

        case Expose://code 7

            if (host_event.xexpose.count != 0)
                break;

            item.x = 0;
            item.y = 0;
            item.button = 0;
            item.event_type = '7';

            input_event_enqueue(item);

            break;

        case DestroyNotify:
            goto input_event_exit;

        default:
            //printf("event %d\n", host_event.type);
            break;
        }
        /* MUST KNOW: We'd better not sleep or miss the hit */
    }

input_event_exit:
    free(arg);
    return NULL;
}

