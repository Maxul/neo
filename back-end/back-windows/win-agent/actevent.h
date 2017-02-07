#include "stdafx.h"

void window_move(Window wid);

void pointer_move(int x, int y, Window windows, int button);

int mouse_click(int x, int y, Window window, int button, int is_press);

int key_press(int x, int y, unsigned int keycode, int is_press);

int window_activate(Window wid);

void window_resize(Window win, int width, int height);

void* act_event(void* arg);