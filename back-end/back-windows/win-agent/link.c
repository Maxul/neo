#include "link.h"

#include <stdio.h>
#include <stdlib.h>
// �������Ϊ��
/*
	ע�����ʧ��ʱ����NULL ������-1
*/

//������
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
//��ֵ���� ����λ�� not find: -1
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

//��ֵ���� ����node not find: NULL
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

// �ҵ�checked==0 ����λ�� not find: -1
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

//����Ԫ��
void InsertItem(Node *head, int loc, Window num)
{
	if (head == NULL)
	{
		return;
	}

	if (loc > Count_Node(head) || loc < 0)
	{
		printf("����λ�ô���\n");
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
//ɾ��Ԫ��
Window deleteItem(Node *head, int loc, int flag)
{
	if (head == NULL)
	{
		return NULL;
	}

	if (loc > Count_Node(head) || loc <= 0)
	{
		printf("ɾ��λ�ô���%d\n", loc);
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

//�������
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

//��������
void DestoryLink(Node *head)
{
	free(head);
}

//����Ų���
Window FindByNo(Node *head, int loc)
{
	if (head == NULL)
	{
		return NULL;
	}

	if (loc > Count_Node(head) || loc <= 0)
	{
		printf("λ�ô���\n");
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

//����Ų���node
Node *FindNodeByNo(Node *head, int loc)
{
	if (head == NULL)
	{
		return NULL;
	}
	if (loc > Count_Node(head) || loc <= 0)
	{
		printf("λ�ô���\n");
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

//β�巨������ͷ���ĵ�����
Node* Creat_Node()
{
	Node *head;
	head = (Node*)malloc(sizeof(Node));;
	head->next = NULL;
	return head;
}

//��ӡ������ ����
void Print_Node(Node *head)
{
	if (head == NULL)
	{
		return;
	}

	Node *p;
	
	p = head->next;
	printf("���������:");
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
