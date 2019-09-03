#include <linux/module.h>
#include <linux/pci.h>
#include <linux/version.h>
#include <linux/semaphore.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <asm/io.h> 
#include <asm/uaccess.h>

#include <share/mkcdev.h>

#define DEV_NAME "pcie_dev"
struct pcie_test_dev {
	struct cdev_struct pci_dev;
	void __iomem *membase;
	char buf[1024];
	struct semaphore sem;
};

static struct pcie_test_dev *test_dev;


static int pcie_open(struct inode *inode, struct file *file)
{
	file->private_data = container_of(inode->i_cdev, struct pcie_test_dev, pci_dev.cdev);

	return 0;
}

static int pcie_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;

	return 0;
}

static ssize_t pcie_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	int i, rc;
	uint32_t *tmp;
	struct pcie_test_dev *dev = file->private_data;

	if (count  > 1024)
		return -ENOMEM;

	down_interruptible(&dev->sem);

	if (copy_from_user(dev->buf, buf, count)) {
		rc = -EFAULT;
		goto err;
	}

//	tmp = (uint32_t *)dev->buf;
//	for (i = 0; i < count / 4 + (count % 4 ? 1 : 0); i++)
//		writel(tmp[i], dev->membase + 0xFFFFF);

	rc = count;
err:
	up(&dev->sem);

	return rc;
}

static ssize_t pcie_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	uint32_t *tmp;
	int i, rc;
	struct pcie_test_dev *dev = file->private_data;

	if (count + *ppos > 1024)
		return -ENOMEM;

	down_interruptible(&dev->sem);

//	tmp = (uint32_t *)dev->buf;
//	for (i = 0; i < count / 4 + (count % 4 ? 1 : 0); i++)
//		tmp[i] = readl(dev->membase + 0xFFFFF);

	if (copy_to_user(buf, dev->buf, count)) {
		rc = -EFAULT;
		goto err;
	}

	rc = count;
err:
	up(&dev->sem);
	return rc;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = pcie_open,
	.write = pcie_write,
	.read = pcie_read,
//	.unlocked_ioctl = pcie_ioctl,
	.release = pcie_release,
};

static int pcie_probe(struct pci_dev *pdev,const struct pci_device_id *ent)
{
	int rc;
	rc = pci_enable_device_mem(pdev);
	if (rc) {
		dev_err(&pdev->dev, "can not enable device memory\n");
		goto err;
	}

	rc = pci_request_regions(pdev, DEV_NAME);
	if (rc) {
		dev_err(&pdev->dev, "can not request region\n");
		goto err_disable_device;
	}

	pci_set_master(pdev);
	rc = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
	if (rc) {
		dev_err(&pdev->dev, "no usable DMA configuration, aborting\n");
		goto err_release_region;
	}

	test_dev->membase = ioremap_nocache(pci_resource_start(pdev, 0),pci_resource_len(pdev, 0));
	if (!test_dev->membase) {
		dev_err(&pdev->dev, "can not ioremap memory\n");
		goto err_release_region;
	}

	pci_set_drvdata(pdev, test_dev);
	/*
	 *
	 *
	 *
	 *
	 * Use membase for register operations
	 *
	 *
	 *
	 *
	 */

	return 0;

err_release_region:
	pci_release_regions(pdev);
err_disable_device:
	pci_disable_device(pdev);
err:
	return -EFAULT;
}

static void pcie_remove(struct pci_dev *pdev)
{
	struct pcie_test_dev *test_dev = pci_get_drvdata(pdev);

	iounmap(test_dev->membase);	
	pci_release_regions(pdev);
	pci_disable_device(pdev);
}

static struct pci_device_id pci_tbl[] = {
	{ PCI_DEVICE(0xFFFF, 0xFFFF), 0, 0, 0 },
	{},
};
MODULE_DEVICE_TABLE(pci, pci_tbl);

static struct pci_driver test_driver = {
	.name = DEV_NAME,
	.id_table = pci_tbl,
	.probe = pcie_probe,
	.remove = pcie_remove,
};

static int __init pcie_test_init(void)
{
	int rc = 0;
	test_dev = kzalloc(sizeof(*test_dev), GFP_KERNEL);
	if (!test_dev)
		return -ENOMEM;

	rc = pci_register_driver(&test_driver);
	if (rc)
		goto out;

	mkcdev(DEV_NAME, 1, &test_dev->pci_dev, &fops, test_dev);
	sema_init(&test_dev->sem, 1);

out:
	return rc;
}

static void __exit pcie_test_exit(void)
{
	delcdev(&test_dev->pci_dev);
	pci_unregister_driver(&test_driver);
	kfree(test_dev);
}

module_init(pcie_test_init);
module_exit(pcie_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("qtl <qtl_linux@163.com>");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("pcie driver test");
