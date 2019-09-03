#ifndef __SHARE_MKCDEV_H_19910612
#define __SHARE_MKCDEV_H_19910612

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>

struct cdev_struct {
	struct cdev 	cdev;
	struct device 	*device;
	struct class 	*class;
	dev_t 		devid;
	unsigned int	cdev_num;
};/* char dev of base class */

extern int mkcdev(const char *name, unsigned int cdev_num,
		struct cdev_struct *cdev_pt, struct file_operations *fops, void *private_data);

extern void delcdev(struct cdev_struct *cdev_pt);

#endif
