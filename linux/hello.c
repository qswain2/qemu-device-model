#include <linux/device.h>
#include <linux/module.h>
#include <linux/pci.h>


static int hello_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	dev_info(&pdev->dev, "Hello device bound\n");
	return 0;
}

static void hello_remove(struct pci_dev *pdev)
{
	dev_info(&pdev->dev, "Hello device removed\n");
}

struct pci_device_id hello_id_table[] = {
	{PCI_DEVICE(0x1337,0x0001)},
	{}
};

struct pci_driver hello_pci_driver = {
	.probe = hello_probe,
	.remove = hello_remove,
	.id_table = hello_id_table
};

static int hello_init(void)
{
	return 0;
}

static void hello_exit(void)
{
	return;
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_DEVICE_TABLE(pci, hello_id_table);
