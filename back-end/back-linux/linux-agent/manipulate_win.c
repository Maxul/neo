#include "manipulate_win.h"
#include "list.h"

#include <regex.h>
#include <stdio.h>
#include <pthread.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/xpm.h>

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

extern Display *dpy;
extern pthread_mutex_t mutex_actwin;

extern Node *head, *head_new;
extern pthread_mutex_t mutex_link;

/*list visual window*/
static int list_view_windows(Display *disp);

/*list pop windows*/
static void list_pop_windows(Display *dpy, Window w);

void enum_new_windows(Display *disp)
{
    Window root = XDefaultRootWindow(disp);

    list_view_windows(disp); // windows with frame
    list_pop_windows(disp, root); // windows without frame
}

/* compile a specific regular expression based on a pattern */
static int compile_re(const char *pattern, regex_t *re)
{
    int ret;

    if (pattern == NULL) {
        regcomp(re, "^$", REG_EXTENDED | REG_ICASE);
        return True;
    }

    ret = regcomp(re, pattern, REG_EXTENDED | REG_ICASE);
    if (0 != ret) {
        fprintf(stderr, "Failed to compile regex (return code %d): '%s'\n", ret, pattern);
        return False;
    }
    return True;
}

/* Does a window's name match a specified pattern */
static int do_window_match_name(Display *dpy, Window window, regex_t *re)
{
    int i;
    int count = 0;
    char **list = NULL;
    XTextProperty tp;

    XGetWMName(dpy, window, &tp);
    if (tp.nitems > 0) {
        Xutf8TextPropertyToTextList(dpy, &tp, &list, &count);
        for (i = 0; i < count; i++) {
            if (0 == regexec(re, list[i], 0, NULL, 0)) {
                XFreeStringList(list);
                XFree(tp.value);
                return True;
            }
        }
    } else {
        /* Treat windows with no names as empty strings */
        if (regexec(re, "", 0, NULL, 0) == 0) {
            XFreeStringList(list);
            XFree(tp.value);
            return True;
        }
    }
    XFreeStringList(list);
    XFree(tp.value);
    return False;
}

/* list visual window */
static int list_view_windows(Display *disp)
{
    int i;
    regex_t name_re;
    Atom prop = XInternAtom(disp, "_NET_CLIENT_LIST", False), type;

    int form;

    unsigned long remain, len;
    unsigned char *list;

    if (!compile_re("Desktop", &name_re)) {
        perror("comlile rex error!\n");
        return 0;
    }

    if (XGetWindowProperty(disp, XDefaultRootWindow(disp), prop, 0, 1024, False,
                           XA_WINDOW, &type, &form, &len, &remain, &list) != Success) {
        return -1;
    }

    pthread_mutex_lock(&mutex_link);
    /* FIXME: Why 4??? */
    for (i = 4; i < len; i++) {
        if (!do_window_match_name(disp, ((Window *)list)[i], &name_re)) {
            /* only those that are not in list will be inserted */
            if (FindByValue(head, ((Window *)list)[i]) == -1) {
                InsertItem(head, 0, ((Window *)list)[i]);
                InsertItem(head_new, 0, ((Window *)list)[i]);
            }

        }
    }
    pthread_mutex_unlock(&mutex_link);
    regfree(&name_re);
    XFree(list);
    return 0;
}

/*list pop windows*/
static void list_pop_windows(Display *dpy, Window w)
{
    int i = 0;

    Window root;
    Window parent_return;
    Window *children_return;

    unsigned int nchildren_return;
    XQueryTree(dpy, w, &root, &parent_return, &children_return, &nchildren_return);

    char *window_name_return;
    XWindowAttributes win_attributes;

    for (i = 0; i < nchildren_return; i++) {

        XFetchName(dpy, children_return[i], &window_name_return);
        XGetWindowAttributes(dpy, children_return[i], &win_attributes);

        if (win_attributes.override_redirect &&
                win_attributes.map_state == IsViewable &&
                window_name_return != NULL &&
                win_attributes.x >= 0) {

            pthread_mutex_lock(&mutex_link);
            /* only those that are not in list will be inserted */
            if (FindByValue(head, children_return[i]) == -1) {
                InsertItem(head, 0, children_return[i]);
                InsertItem(head_new, 0, children_return[i]);
            }
            pthread_mutex_unlock(&mutex_link);
        }
        XFree(window_name_return);
    }
    XFree(children_return);
}

static unsigned char *get_window_property_by_atom(Display *dpy,
        Window window,
        Atom atom,
        long *nitems,
        Atom *type,
        int *size)
{
    Atom actual_type;
    int actual_format;
    unsigned long _nitems;
    unsigned long bytes_after;
    unsigned char *prop;

    XGetWindowProperty(dpy, window, atom, 0, (~0L),
                       False, AnyPropertyType, &actual_type,
                       &actual_format, &_nitems, &bytes_after,
                       &prop);

    if(nitems != NULL)
        *nitems = _nitems;

    if(type != NULL)
        *type = actual_type;

    if(size != NULL)
        *size = actual_format;

    return prop;
}

void get_relate_window(Display *dpy, Window *window_ret)
{
    Atom type;
    int size;
    long nitems;
    unsigned char *data;
    Atom request;
    Window root;

    request = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
    root = XDefaultRootWindow(dpy);
    data = get_window_property_by_atom(dpy, root, request, &nitems, &type, &size);

    if (nitems > 0)
        *window_ret = *((Window*)data);
    else
        *window_ret = 0;
    free(data);
}

