#ifndef CLIENT_LINK_H_
#define CLIENT_LINK_H_

#include <X11/Xlib.h>

typedef struct list_node {
    int close;
    Window wid;
    struct list_node *next;
} Node;

//链表长度
int Count_Node(Node *linkList);

//按值查找 返回位置 not find: -1
int FindByValue(Node *linkList, Window v);

//按值查找 返回node not find: NULL
Node *FindNodeByValue(Node *linkList, Window v);

// 找到checked==0 返回位置 not find: -1
int FindUnhecked(Node *linkList);

//插入元素
void InsertItem(Node *linkList, int loc, Window num);

//删除元素
Window deleteItem(Node *linkList, int loc);

//清空链表
void FlushLink(Node *linkList);

//销毁链表
void DestoryLink(Node *linkList);

//按序号查找
Window FindByNo(Node *linkList, int loc);

//按序号查找node
Node *FindNodeByNo(Node *linkList, int loc);

//尾插法建立带头结点的单链表
Node* Creat_Node();

//打印单链表 遍历
void Print_Node(Node *linkList);

#endif
