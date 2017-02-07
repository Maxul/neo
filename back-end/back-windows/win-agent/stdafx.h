// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
//

#pragma once

// TODO:  �ڴ˴����ó�����Ҫ������ͷ�ļ�
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <winioctl.h>
#include <stdlib.h>
#include <string.h>
#include <strsafe.h>

#define Window HWND

typedef struct List_Node {
	Window wid;
	int flag;
	int checked;
	int close;
	struct List_Node *next;
} Node;

typedef struct {
	int x, y, button;
	char type;
	Window source_wid;
} Item;

typedef struct node
{
	Item data;
	struct node *next;
} *PNode;

typedef struct
{
	PNode front;
	PNode rear;
	int size;
} Queue;

typedef struct {
	Window wid;
} arg_struct;
