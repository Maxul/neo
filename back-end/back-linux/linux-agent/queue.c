#include"queue.h"

#include<stdio.h>
#include<malloc.h>

static int IsEmpty(Queue *pqueue)
{
    if (pqueue->front == NULL && pqueue->rear == NULL && pqueue->size == 0)
        return 1;
    else
        return 0;
}

static void ClearQueue(Queue *pqueue)
{
    while (!IsEmpty(pqueue))
        DeQueue(pqueue, NULL);
}

Queue *InitQueue()
{
    Queue *pqueue = malloc(sizeof(Queue));

    if (pqueue) {
        pqueue->front = NULL;
        pqueue->rear = NULL;
        pqueue->size = 0;
    }
    return pqueue;
}

void DestroyQueue(Queue *pqueue)
{
    if (!IsEmpty(pqueue))
        ClearQueue(pqueue);

    free(pqueue);
}

int GetSize(Queue *pqueue)
{
    return pqueue->size;
}

PNode EnQueue(Queue *pqueue, Item item)
{
    PNode pnode = malloc(sizeof(qNode));
    if (pnode) {
        pnode->data = item;
        pnode->next = NULL;

        if (IsEmpty(pqueue))
            pqueue->front = pnode;
        else
            pqueue->rear->next = pnode;

        pqueue->rear = pnode;
        pqueue->size++;
    }
    return pnode;
}

PNode DeQueue(Queue *pqueue, Item *pitem)
{
    PNode pnode = pqueue->front;

    if (!IsEmpty(pqueue) && pnode != NULL) {

        if (pitem != NULL)
            *pitem = pnode->data;

        pqueue->size--;
        pqueue->front = pnode->next;
        free(pnode);

        if (pqueue->size == 0)
            pqueue->rear = NULL;
    }
    return pqueue->front;
}

#if 0
static PNode GetFront(Queue *pqueue, Item *pitem)
{
    if (!IsEmpty(pqueue) && pitem != NULL)
        *pitem = pqueue->front->data;

    return pqueue->front;
}

static PNode GetRear(Queue *pqueue, Item *pitem)
{
    if (!IsEmpty(pqueue) && pitem != NULL)
        *pitem = pqueue->rear->data;

    return pqueue->rear;
}
#endif

