#ifndef CLIENT_COMU_H_
#define CLIENT_COMU_H_

#include <X11/Xlib.h>

void *recv_cmd(void *unused);

/*receive event to queue*/
void *recv_events(void *arg);

/*send close notify to qemu-kvm*/
void send_notify(Window win);

#endif
