#include "list.h"

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>

void Print_Node(Node *linkList)
{
    Node *p;

    if (linkList == NULL)
        return;

    p = linkList->next;
    while (p) {
        printf("%lx -> ", p->wid);
        p = p->next;
    }
    puts("^");
}

Node* Creat_Node(void)
{
    Node *linkList;

    linkList = malloc(sizeof(Node));
    linkList->next = NULL;
    return linkList;
}

void FlushLink(Node *linkList)
{
    Node *p;

    if (linkList == NULL)
        return;

    while ((p = linkList->next) != NULL) {
        linkList->next = p->next;
        free(p);
    }
}

void DestoryLink(Node *linkList)
{
    FlushLink(linkList);
    free(linkList);
}

int Count_Node(Node *linkList)
{
    int num;
    Node *p;

    if (linkList == NULL)
        return -1;

    num = 0;
    p = linkList->next;
    while (p != NULL) {
        num++;
        p = p->next;
    }
    return num;
}

int FindByValue(Node *linkList, Window val)
{
    int i;
    Node *p;

    if (linkList == NULL)
        return -1;

    i = 0;
    p = linkList->next;
    while (p != NULL) {
        i++;
        if (p->wid == val)
            return i;
        p = p->next;
    }
    return -1;
}

void InsertItem(Node *linkList, int pos, Window win)
{
    Node *p, *pre, *node;
    int i = 0;

    if (pos > Count_Node(linkList) || pos < 0) {
        fprintf(stderr, "InsertItem: pos err\n");
        return;
    }
    if (linkList == NULL) {
        fprintf(stderr, "InsertItem: link is null\n");
        return;
    }

    pre = linkList;
    p = linkList->next;
    while (i != pos) {
        i++;
        pre = p;
        p = p->next;
    }

    node = malloc(sizeof(Node));
    node->close = 0;
    node->wid = win;

    /* insert the node */
    node->next = p;
    pre->next = node;
}

Window deleteItem(Node *linkList, int pos)
{
    Node *p, *pre;
    Window win = 0;
    int i = 1;

    if (pos > Count_Node(linkList) || pos <= 0) {
        fprintf(stderr, "deleteItem: pos err\n");
        return -1;
    }
    if (linkList == NULL) {
        fprintf(stderr, "deleteItem: link is null\n");
        return -1;
    }

    pre = linkList;
    p = pre->next;
    while (i != pos) {
        i++;
        pre = p;
        p = p->next;
    }

    win = p->wid;
    pre->next = p->next;
    p->next = NULL;
    free(p);
    return win;
}

