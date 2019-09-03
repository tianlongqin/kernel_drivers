#include <linux/version.h>
#include <share/mkcdev.h>

int mkcdev(const char *name, unsigned int cdev_num,
		struct cdev_struct *cdev_pt, struct file_operations *fops, void *private_data)
{
	int rc, i;
	char __name[32];

	cdev_num = cdev_num ? : 1; /* if cdev_num == 0. cdev_num = 1; */
	cdev_pt->class = class_create(THIS_MODULE, name);
	cdev_init(&cdev_pt->cdev, fops);
	rc = alloc_chrdev_region(&cdev_pt->devid, 0, cdev_num, name);
	if (rc) {
		printk(KERN_ERR "%s: can't alloc chrdev region\n", name);
		goto err_chrdev;
	}
	for (i = 0; i < cdev_num; i++) {
		if (cdev_num != 1)
			snprintf(__name, 31, "%s.%d", name, i);
		else
			snprintf(__name, 31, "%s", name);

		cdev_pt->device = device_create(cdev_pt->class,
						NULL,
						MKDEV(MAJOR(cdev_pt->devid), i),
						private_data, __name);
		if (IS_ERR(cdev_pt->device)) {
			rc = PTR_ERR(cdev_pt->device);
			goto err_device;
		}
	}

	rc = cdev_add(&cdev_pt->cdev, cdev_pt->devid, cdev_num);
	if (rc) {
		printk(KERN_ERR "%s: can't add cdev\n", __name);
		goto err_cdev;
	}

	cdev_pt->cdev_num = cdev_num;

	return 0;

err_cdev:
	for (i = 0; i < cdev_num; i++)
		device_destroy(cdev_pt->class, MKDEV(MAJOR(cdev_pt->devid), i));
err_device:
	unregister_chrdev_region(cdev_pt->devid, cdev_num);
err_chrdev:
	cdev_del(&cdev_pt->cdev);
	class_destroy(cdev_pt->class);

	return rc;
}

void delcdev(struct cdev_struct *cdev_pt)
{
	int i;
	if (!cdev_pt)
		return ;

	for (i = 0; i < cdev_pt->cdev_num; i++)
		device_destroy(cdev_pt->class, MKDEV(MAJOR(cdev_pt->devid), i));
	unregister_chrdev_region(cdev_pt->devid, cdev_pt->cdev_num);
	cdev_del(&cdev_pt->cdev);
	class_destroy(cdev_pt->class);
}
