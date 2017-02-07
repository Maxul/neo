#include "search.h"

#include <stdio.h>
#include <X11/xpm.h>

extern int desktop;

int do_window_match_name(Display *dpy, Window window, regex_t *re)
{
    int i;
    int count = 0;
    char **list = NULL;
    XTextProperty tp;

    XGetWMName(dpy, window, &tp);
    if (tp.nitems > 0)
    {
        Xutf8TextPropertyToTextList(dpy, &tp, &list, &count);
        for (i = 0; i < count; i++)
        {
            if (regexec(re, list[i], 0, NULL, 0) == 0)
            {
                XFreeStringList(list);
                XFree(tp.value);
                return True;
            }
        }
    }
    else
    {
        /* Treat windows with no names as empty strings */
        if (regexec(re, "", 0, NULL, 0) == 0)
        {
            XFreeStringList(list);
            XFree(tp.value);
            return True;
        }
    }
    XFreeStringList(list);
    XFree(tp.value);
    return False;
}

int compile_re(const char *pattern, regex_t *re)
{
    int ret;
    if (pattern == NULL)
    {
        regcomp(re, "^$", REG_EXTENDED | REG_ICASE);
        return True;
    }

    ret = regcomp(re, pattern, REG_EXTENDED | REG_ICASE);
    if (ret != 0)
    {
        fprintf(stderr, "Failed to compile regex (return code %d): '%s'\n", ret, pattern);
        return False;
    }
    return True;
}

void get_window(Display *dpy, Window w, const char *winname)
{
    int i = 0;
    Window root;
    Window parent_return;
    Window *children_return;
    unsigned int nchildren_return;
    regex_t name_re;
    if (!compile_re(winname, &name_re))
    {
        perror("comlile rex error!\n");
        return;
    }
    XQueryTree(dpy, w, &root, &parent_return, &children_return, &nchildren_return);
    for (i = 0; i < nchildren_return; i++)
    {
        if (do_window_match_name(dpy, children_return[i], &name_re))
            desktop = children_return[i];
        get_window(dpy, children_return[i], winname);
    }
    regfree(&name_re);
    XFree((void *)(children_return));
}

