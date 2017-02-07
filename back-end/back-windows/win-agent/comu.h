#include "stdafx.h"

/*receive event to queue*/
void *receive_queue(void *arg);

/*send close notify to qemu-kvm*/
void send_notify(Window win);