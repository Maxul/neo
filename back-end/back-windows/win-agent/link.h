#include "stdafx.h"

//������
int Count_Node(Node *head);

//��ֵ���� ����λ�� not find: -1
int FindByValue(Node *head, Window id);

//��ֵ���� ����node not find: NULL
Node *FindNodeByValue(Node *head, Window id);

// �ҵ�checked==0 ����λ�� not find: -1
int FindUnhecked(Node *head);

//����Ԫ��
void InsertItem(Node *head, int loc, Window num);

//ɾ��Ԫ��
Window deleteItem(Node *head, int loc, int flag);

//�������
void FlushLink(Node *head);

//��������
void DestoryLink(Node *head);

//����Ų���
Window FindByNo(Node *head, int loc);

//����Ų���node
Node *FindNodeByNo(Node *head, int loc);

//β�巨������ͷ���ĵ�����
Node* Creat_Node();

//��ӡ������ ����
void Print_Node(Node *head);
