#include "stdafx.h"

/*thread for listen*/
void* file_listen(void* arg);
void* device_listen(void* arg);
int init_hotplug_sock(void);