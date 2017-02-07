#ifndef _MEMDEV_H_
#define _MEMDEV_H_

#ifndef MEMDEV_NR_DEVS
#define MEMDEV_NR_DEVS (1)
#endif

#ifndef MEMDEV_NAME
#define MEMDEV_NAME "dev_mem"
#endif

#ifndef MEMDEV_SIZE
#define MEMDEV_SIZE ((1<<20)*4)
#endif

#define MAGIC "TRUST"

#endif /* _MEMDEV_H_ */
