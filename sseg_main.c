/*
 * Seven-Segment Driver
 *
 * Copyright (C) 2014 Thomas Kolb
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/slab.h>

#include <linux/uaccess.h>

#include "signalling.h"
#include "config.h"


MODULE_AUTHOR("Thomas Kolb");
MODULE_DESCRIPTION("7-Segment display driver");
MODULE_LICENSE("GPL");

dev_t dev = 0;
struct class *cl;
struct cdev *sseg_cdev;

int is_open = 0;

static ssize_t sseg_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset);
static int sseg_open(struct inode *inode, struct file *file);
static int sseg_release(struct inode *inode, struct file *file);
static ssize_t sseg_write(struct file *filp, const char *buff, size_t len, loff_t *off);

struct file_operations sseg_fops = {
	.read = sseg_read,
	.write = sseg_write,
	.open = sseg_open,
	.release = sseg_release,
	.owner = THIS_MODULE
};


static int __init sseg_init(void)
{
#if DEBUG
	printk(KERN_INFO "sseg: Welcome!");
#endif // DEBUG

	// /proc/devices
	if (alloc_chrdev_region(&dev, 0, 1, "sseg") < 0) {
		goto fail;
	}

#if DEBUG
	printk(KERN_INFO "sseg: alloc_chrdev_region successful");
#endif // DEBUG

	// /sys/class
	if ((cl = class_create(THIS_MODULE, "seven_seg")) == NULL) {
		goto fail_class;
	}

#if DEBUG
	printk(KERN_INFO "sseg: class_create successful");
#endif // DEBUG

	// device in /dev
	if (device_create(cl, NULL, dev, NULL, "sseg") == NULL) {
		goto fail_device;
	}

#if DEBUG
	printk(KERN_INFO "sseg: device_create successful");
#endif // DEBUG

	sseg_cdev = cdev_alloc();
	if(sseg_cdev == NULL) {
		goto fail_cdev_alloc;
	}

#if DEBUG
	printk(KERN_INFO "sseg: cdev_alloc successful");
#endif // DEBUG

	sseg_cdev->ops = &sseg_fops;
	sseg_cdev->owner = THIS_MODULE;

	if (cdev_add(sseg_cdev, dev, 1) == -1) {
		goto fail_cdev_add;
	}

#if DEBUG
	printk(KERN_INFO "sseg: cdev_add successful");
#endif // DEBUG

	if(init_signalling() != 0) {
		goto fail_signalling;
	}

	signalling_showtext(" . . . . .", 10);

	printk(KERN_INFO "sseg: Initialization complete");

	return 0;

fail_signalling:
fail_cdev_add:
	cdev_del(sseg_cdev); // Beware: remove on kernel panic!
fail_cdev_alloc:
	device_destroy(cl, dev);
fail_device:
	class_destroy(cl);
fail_class:
	unregister_chrdev_region(dev, 1);
fail:

	return -1;
}

static void __exit sseg_cleanup(void)
{
	printk("Bye\n");

	signalling_showtext("", 0);

	if(dev != 0) {
		cleanup_signalling();
		cdev_del(sseg_cdev);
		device_destroy(cl, dev);
		class_destroy(cl);
		unregister_chrdev_region(dev, 1);
	}

	return;
}

static int sseg_open(struct inode *inode, struct file *file)
{
	if(!is_open) {
#if DEBUG
		printk("sseg: Device opened\n");
#endif // DEBUG
		is_open = 1;
		return 0; // success
	} else {
#if DEBUG
		printk("sseg: Device is already open\n");
#endif // DEBUG
		return -EBUSY; // already open
	}
}

static int sseg_release(struct inode *inode, struct file *file)
{
#if DEBUG
	printk("sseg: Device closed\n");
#endif // DEBUG
	is_open = 0;
	return 0; // success
}

static ssize_t sseg_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset)
{
#if DEBUG
	printk(KERN_ALERT "Sorry, this operation isn't supported.\n");
#endif // DEBUG
	return -EINVAL;
}

static ssize_t sseg_write(struct file *filp, const char __user *buffer, size_t len, loff_t *off)
{
	char *kBuf;
	size_t kLen;

	if(len > 2*nseg) {
		kLen = 2*nseg;
	} else {
		kLen = len;
	}

	kBuf = kmalloc(kLen+1, GFP_KERNEL);
	if(!kBuf) {
#if DEBUG
		printk(KERN_ALERT "sseg: kmalloc failed");
#endif // DEBUG
		return -ENOMEM;
	}

	if(copy_from_user(kBuf, buffer, kLen) != 0) {
#if DEBUG
		printk(KERN_ALERT "sseg: copy_from_user failed to copy everything");
#endif // DEBUG
		return -EFAULT;
	}

	while((kLen > 0) && ((kBuf[kLen-1] == '\n') || (kBuf[kLen-1] == '\r'))) {
		kLen--;
	}

	kBuf[kLen] = '\0';

#if DEBUG
	printk(KERN_INFO "sseg: wrote %d bytes: %s\n", len, kBuf);
#endif // DEBUG

	signalling_showtext(kBuf, len);

	kfree(kBuf);
	return len;
}

module_init(sseg_init);
module_exit(sseg_cleanup);

