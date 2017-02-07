#include "stdafx.h"

/*����һ���ն���*/
Queue *InitQueue();

/*����һ������*/
void DestroyQueue(Queue *pqueue);

/*���һ������*/
void ClearQueue(Queue *pqueue);

/*�ж϶����Ƿ�Ϊ��*/
int IsEmpty(Queue *pqueue);

/*���ض��д�С*/
int GetSize(Queue *pqueue);

/*���ض�ͷԪ��*/
PNode GetFront(Queue *pqueue, Item *pitem);

/*���ض�βԪ��*/
PNode GetRear(Queue *pqueue, Item *pitem);

/*����Ԫ�����*/
PNode EnQueue(Queue *pqueue, Item item);

/*��ͷԪ�س���*/
PNode DeQueue(Queue *pqueue, Item *pitem);

