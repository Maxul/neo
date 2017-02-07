#include "memctl.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>

void *openRAM(void)
{
    /* open RAM */
    int fd = open("/dev/mem", O_RDWR);
    if (fd < 0)
    {
        perror("/dev/mem");
        exit(1);
    }

    /* map low memory */
    void *mem = mmap(LOW_ADDR, LOW_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mem == MAP_FAILED)
    {
        perror("mmap low memory error");
        exit(1);
    }

    return mem;
}

void *openKernelMemory(void)
{
    /* open our memory device */
    int fd = open("/dev/dev_mem", O_RDWR);
    if (fd < 0)
    {
        perror("open /dev/dev_mem");
        exit(1);
    }

    /* map kernel memory */
    void *kernel_mem = mmap(NULL, SHAREDSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (kernel_mem == MAP_FAILED)
    {
        perror("mmap kernel memory error");
        exit(1);
    }
    return kernel_mem;
}

