#include "link.h"

#include <stdio.h>
#include <stdlib.h>
// 建议设计为类
/*
	注意查找失败时返回NULL 而不是-1
*/

//链表长度
int Count_Node(Node *head)
{
	if (head == NULL)
	{
		return -1;
	}

	Node *p;
	int num = 0;
	
	p = head->next;
	while (p != NULL)
	{
		num++;
		p = p->next;
	}

	return num;
}
//按值查找 返回位置 not find: -1
int FindByValue(Node *head, Window id)
{
	if (head == NULL)
	{
		return -1;
	}

	Node *p;
	int i = 0;
	p = head->next;
	while (p != NULL)
	{
		i++;

		if (p->wid == id)
		{
			return i;
		}

		p = p->next;
	}

	return -1;
}

//按值查找 返回node not find: NULL
Node *FindNodeByValue(Node *head, Window id)
{
	if (head == NULL)
	{
		return NULL;
	}

	Node *p;
	int i = 0;
	
	p = head->next;
	while (p != NULL)
	{
		i++;

		if (p->wid == id)
		{
			return p;
		}

		p = p->next;
	}

	return NULL;
}

// 找到checked==0 返回位置 not find: -1
int FindUnhecked(Node *head)
{
	if (head == NULL)
	{
		return -1;
	}

	Node *p;
	int i = 0;
	
	p = head->next;
	while (p != NULL)
	{
		i++;

		if (p->checked == 0)
		{
			return i;
		}
		p = p->next;
	}
	return -1;
}

//插入元素
void InsertItem(Node *head, int loc, Window num)
{
	if (head == NULL)
	{
		return;
	}

	if (loc > Count_Node(head) || loc < 0)
	{
		printf("插入位置错误\n");
		return;
	}

	Node *p, *pre, *s;
	int i = 0;

	pre = head;
	p = head->next;
	while (i != loc)
	{
		i++;
		pre = p;
		p = p->next;
	}
	s = (Node*)malloc(sizeof(Node));
	s->wid = num;
	s->next = p;
	s->checked = 1;
	s->close = 0;

	pre->next = s;
}
//删除元素
Window deleteItem(Node *head, int loc, int flag)
{
	if (head == NULL)
	{
		return NULL;
	}

	if (loc > Count_Node(head) || loc <= 0)
	{
		printf("删除位置错误%d\n", loc);
		return NULL;
	}

	Node *p, *pre;
	int i = 1;
	Window num = 0;

	pre = head;
	p = pre->next;
	while (i != loc)
	{
		i++;
		pre = p;
		p = p->next;
	}
	num = p->wid;
	pre->next = p->next;
	p->next = NULL;
	free(p);
	return num;
}

//清空链表
void FlushLink(Node *head)
{
	if (head == NULL)
	{
		return;
	}

	Node *p;
	
	while ((p = head->next) != NULL) {
		head->next = p->next;
		free(p);
	}
}

//销毁链表
void DestoryLink(Node *head)
{
	free(head);
}

//按序号查找
Window FindByNo(Node *head, int loc)
{
	if (head == NULL)
	{
		return NULL;
	}

	if (loc > Count_Node(head) || loc <= 0)
	{
		printf("位置错误\n");
		return NULL;
	}

	Node *p;
	int i = 1;

	p = head->next;
	while (i != loc)
	{
		i++;
		p = p->next;
	}
	return p->wid;
}

//按序号查找node
Node *FindNodeByNo(Node *head, int loc)
{
	if (head == NULL)
	{
		return NULL;
	}
	if (loc > Count_Node(head) || loc <= 0)
	{
		printf("位置错误\n");
		return NULL;
	}

	Node *p;
	int i = 1;

	p = head->next;
	while (i != loc)
	{
		i++;
		p = p->next;
	}
	return p;
}

//尾插法建立带头结点的单链表
Node* Creat_Node()
{
	Node *head;
	head = (Node*)malloc(sizeof(Node));;
	head->next = NULL;
	return head;
}

//打印单链表 遍历
void Print_Node(Node *head)
{
	if (head == NULL)
	{
		return;
	}

	Node *p;
	
	p = head->next;
	printf("输出该链表:");
	while (p)
	{
		printf("0x%x--->", (unsigned int)p->wid);
		p = p->next;
	}
	if (p == NULL)
	{
		printf("^\n");
	}
}
