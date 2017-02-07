#ifndef CLIENT_QUEUE_H_
#define CLIENT_QUEUE_H_

#include <stdint.h>

typedef struct {
    int64_t x, y, button;
    int64_t source_wid;
    char type;
} Item;

typedef struct node * PNode;

typedef struct node {
    Item data;
    PNode next;
} qNode;

typedef struct {
    int size;
    PNode front;
    PNode rear;
} Queue;

/*构造一个空队列*/
Queue *InitQueue();

/*销毁一个队列*/
void Destroyint64_tQueue(Queue *pqueue);

/*返回队列大小*/
int GetSize(Queue *pqueue);

/*将新元素入队*/
PNode EnQueue(Queue *pqueue, Item item);

/*队头元素出队*/
PNode DeQueue(Queue *pqueue, Item *pitem);

#endif
