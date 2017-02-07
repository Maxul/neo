// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

// TODO:  在此处引用程序需要的其他头文件
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
