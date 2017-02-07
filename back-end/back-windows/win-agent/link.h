#include "stdafx.h"

//链表长度
int Count_Node(Node *head);

//按值查找 返回位置 not find: -1
int FindByValue(Node *head, Window id);

//按值查找 返回node not find: NULL
Node *FindNodeByValue(Node *head, Window id);

// 找到checked==0 返回位置 not find: -1
int FindUnhecked(Node *head);

//插入元素
void InsertItem(Node *head, int loc, Window num);

//删除元素
Window deleteItem(Node *head, int loc, int flag);

//清空链表
void FlushLink(Node *head);

//销毁链表
void DestoryLink(Node *head);

//按序号查找
Window FindByNo(Node *head, int loc);

//按序号查找node
Node *FindNodeByNo(Node *head, int loc);

//尾插法建立带头结点的单链表
Node* Creat_Node();

//打印单链表 遍历
void Print_Node(Node *head);
