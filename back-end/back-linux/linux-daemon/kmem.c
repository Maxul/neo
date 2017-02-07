/*
 * kmem.c
 * This file is part of NEO
 *
 * Copyright (C) 2017 - Maxul
 *
 * NEO is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * NEO is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with NEO. If not, see <http://www.gnu.org/licenses/>.
 */


/*
    Original: Ecular <ecular_xu@trendmicro.com.cn>
    Revised:  Maxul  <maxul@bupt.edu.cn>
*/

#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/cdev.h>

#include <linux/fs.h>
#include <linux/mm.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maxul");
MODULE_DESCRIPTION("NEO");

#include "kmem.h"

static int major = 0;

/* 模块参数 */
module_param(major, int, S_IRUGO);

struct cdev chrdev;

static void *guestRAM;

static void *kmem_virt = NULL;

/***************************************************************************
    A long time ago, in v2.4, VM_RESERVED kept swapout process off VMA, currently
it lost original meaning but still has some effects:

     | effect                 | alternative flags
    -+------------------------+---------------------------------------------
    1| account as reserved_vm | VM_IO
    2| skip in core dump      | VM_IO, VM_DONTDUMP
    3| do not merge or expand | VM_IO, VM_DONTEXPAND, VM_HUGETLB, VM_PFNMAP
    4| do not mlock           | VM_IO, VM_DONTEXPAND, VM_HUGETLB, VM_PFNMAP

    Thus VM_RESERVED can be replaced with VM_IO or pair VM_DONTEXPAND | VM_DONTDUMP.
***************************************************************************/
static int memdev_mmap(struct file*filp, struct vm_area_struct *vma)
{
    vma->vm_flags |= VM_IO;
    vma->vm_flags |= (VM_DONTEXPAND | VM_DONTDUMP);

    /* 将 kmem_virt 映射到用户空间 */
    if (remap_pfn_range(vma, vma->vm_start,
                        virt_to_phys(kmem_virt) >> PAGE_SHIFT,
                        vma->vm_end - vma->vm_start,
                        vma->vm_page_prot))
    {
        return  -EAGAIN;
    }
    printk(KERN_INFO "NEO: memdev_mmap\n");
    return 0;
}

/* 文件操作结构体 */
static const struct file_operations device_fops =
{
    .owner = THIS_MODULE,
    .mmap = memdev_mmap,
};

static void taint(void)
{
    guestRAM = phys_to_virt(0x00000000);
    memset(guestRAM, 0x0, 64);
    memcpy(guestRAM, MAGIC, sizeof(MAGIC));
    *(long long unsigned int *)(guestRAM + 0x10) = virt_to_phys(kmem_virt);
}

static void sweep(void)
{
    memset(guestRAM, 0x0, 64);
    guestRAM = kmem_virt = NULL;
}

/* 设备驱动模块加载函数 */
static int __init device_init(void)
{
    int result;
    int err;

    dev_t dev_no;

    if (major) {	/* 静态申请设备号 */
        dev_no = MKDEV(major, 0);
        result = register_chrdev_region(dev_no, MEMDEV_NR_DEVS, MEMDEV_NAME);
    } else {		/* 动态申请设备号 */
        result = alloc_chrdev_region(&dev_no, 0, MEMDEV_NR_DEVS, MEMDEV_NAME);
        major = MAJOR(dev_no);
    }

    if (result < 0) {
        printk(KERN_ERR "NEO: Can't get major %d\n", major);
        return result;
    } else {
        printk(KERN_INFO "NEO: Get major %d\n", major);
    }

    /* 初始化cdev结构 */
    cdev_init(&chrdev, &device_fops);

    /* 注册字符设备 */
    err = cdev_add(&chrdev, dev_no, MEMDEV_NR_DEVS);
    if (err) {
        printk(KERN_WARNING "NEO: Error %d adding %d", err, MINOR(dev_no));
        goto fail;
    }

    /* 分配连续物理内存 */
    kmem_virt = kmalloc(MEMDEV_SIZE, GFP_KERNEL);
    if (kmem_virt == NULL) {
        printk(KERN_ERR "NEO: Allocated Memory Failed.\n");
        goto fail;
    }
    memset(kmem_virt, 'z', MEMDEV_SIZE);

    taint();
    printk(KERN_NOTICE "NEO: Allocated Memory at 0x%llx successfully!\n", virt_to_phys(kmem_virt));
    return 0;

fail:
    unregister_chrdev_region(dev_no, 1);
    return result;
}

/* 模块卸载函数 */
static void __exit device_exit(void)
{
    if (kmem_virt) {
        kfree(kmem_virt);	/* 释放内核内存 */
    }
    cdev_del(&chrdev);		/* 注销设备 */
    unregister_chrdev_region(MKDEV(major, 0), MEMDEV_NR_DEVS);	/* 释放设备号 */

    sweep();
    printk(KERN_NOTICE "NEO: Free Memory successfully!\n");
}

module_init(device_init);
module_exit(device_exit);

