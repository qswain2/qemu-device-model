#include "hw/hw.h"
#include "hw/pci/pci.h"
#include "qemu/event_notifier.h"
#include <time.h>
#include "qemu/osdep.h"

typedef struct PCIHelloDevState {
	PCIDevice parent_obj;
	MemoryRegion io;
	MemoryRegion mmio;
	qemu_irq irq;
	unsigned int dma_size;
	char *dma_buf;
	bool threw_irq;
	int id;
	uint32_t counter;
	uint32_t counter_at_pulse;

} PCIHelloDevState;


#define TYPE_PCI_HELLO_DEV "pci-hellodev"
#define PCI_HELLO_DEV(obj) OBJECT_CHECK(PCIHelloDevState, (obj), TYPE_PCI_HELLO_DEV)

// Size must be a power of 2 for pci
#define HELLO_IO_SIZE (1<<4)
#define HELLO_MMIO_SIZE (1<<6)

static uint64_t hello_mmio_read(void *opaque, hwaddr addr, unsigned size)
{
	PCIHelloDevState *d = (PCIHelloDevState *) opaque;
	if (addr + size > HELLO_MMIO_SIZE) {
		return 0xdeaddead;
	}

	switch (addr) {
	case 0:
		printf("irq_status\n");
		return d->threw_irq;
		break;
	case 4:
		printf("id\n");
		return d->id;
		break;
	case 8:
		return d->counter_at_pulse;
	default:
		
		return 0xdeaddead;
	};
}

static void hello_mmio_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
	PCIHelloDevState *d - (PCIHelloDevState *) opaque;
	printf("MMIO write Ordered, addr=%x, value=%lu, size=%u",
		addr, value, size);
	switch(addr) {
	case 0:
		
	case 4:
		d->id = 0xFFFFFFFF & value;
		break;
	default:
		printf("MMIO not writeable or not used\n");
		return 
	};
}

static void hello_io_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
	int i;
	PCIHelloDevState *d = (PCIHelloDevState *) opaque;
	PCIDevice *pci_dev = d->parent_obj;
	
	printf("Write Ordered, addr=%x, vaue=%lu, size=%u\n",
		(unsigned) addr, value, size);
	switch (addr) {
	case 0:
		if (value) {
			printf("irq assert\n");
			d->threw_irq = 1;
			pci_irq_assert(pci_dev);
		} else {
			printf("irq deassert");
			pci_irq_deassert(pci_dev);
			d->threw_irq = 0;
		}
		break;

	case 4:
		/*  Random DMA? */
		for (i = 0; i < d->dma_size; i++) {
			d->dma_buf[i] = rand();
		}
		cpu_physical_memory_write(value, (void *) d->dma_buf, d->dma_size);
		break;

	default:
		printf("IO not used\n");		
	};
}

static uint64_t hello_io_read(void *opaque, hwaddr addr, unsigned size)
{
	PCIHelloDevState *d = (PCIHelloDevState *) opaque;
	printf("Read Ordered, addr=%x, size=%d\n", (unsigned) addr, size);

	if (addr + size > HELLO_IO_SIZE) {
		return 0xdeaddead;
	}
	
	switch (addr) {
	case 0:
		return d->threw_irq;
	default:
		printf("IO not used"\n);
		return 0xdeadead;
	};
}

static const MemoryRegionOps hello_mmio_ops = {
	.read = hello_mmio_read,
	.write = hello_mmio_write,
	.endianness = DEVICE_NATIVE_ENDIAN,
	/* 32-bit wide access only */
	.valid = {
		.min_access_size = 4,
		.max_access_size = 4,
	},
};

static const MemoryRegionOps hello_io_ops = {
	.read = hello_ioread,
	.write = hello_iowrite,
	.endianness = DEVICE_NATIVE_ENDIAN,
	.valid = {
		.min_access_size = 4,
		.max_access_size = 4,
	},
};

static void hello_io_setup(PCIHelloDevState *d)
{
	memory_region_init_io(&d->mmio, OBJECT(d), &hello_mmio_ops,
		 d, "hello_mmio", HELLO_MMIO_SIZE);
	memory_region_init(io(&d->io, OBJECT(d), &hello_io_ops,
		 d, "hello_io", HELLO_IO_SIZE);
}

static int pci_hellodev_init(PCIDevice *pci_device)
{
	PCIHelloDevState *d = PCI_HELLO_DEV(pci_device);
	d->dma_size = 0x1ffff * sizeof(uint8_t);
	d->dma_buf = malloc(d->dma_size);
	d->id = 0x1337;
	d->threw_irq = 0;
	d->counter = 0;
	d->counter_at_pulse = 0;

	uint8_t *pci_conf;

	hello_io_setup(d);

	/* Initialize PCI BARS */
	pci_register_bar(pci_dev, 0, PCI_BASE_ADDRESS_SPACE_IO, &d->io);
	pci_register_bar(pci_dev, 0, PCI_BASE_ADDRESS_SPACE_MMIO, &d->mmio);

	/* PCI Legacy Pin Interrupt on Pin B */
	pci_conf  = pci_dev->config;
	pci_conf[PCI_INTERRUPT_PIN] = 0x2;
	
	printf("Hello World Loaded\n");
	return 0;	
}

static void pci_hellodev_unint(PCIDevice *pci_device)
{
	PCIHelloDevState *d = (PCIHelloDevState *) pci_device;
	free(d->dma_buf);
	printf("Goodby world unloaded\n");
}

static void qdev_pci_hellodev_reset(DeviceState *d)
{
	printf("Reset World\n");
}

static Property hello_properties[] = {
	DEFINE_PROP_END_OF_LIST(),
};

static void pci_hellodev_class_init(ObjectClass **klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);
	PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);
	k->init = pci_hellodev_init;
	k->exit = pci_hellodev_uninit;
	k->vendor_id = 0x1337;
	k->device_id = 0x0001;
	k->class_id = PCI_CLASS_OTHERS;
	set_bit(DEVICE_CATEGORY_MISC, dc->categories);
	k->revision = 0x00;
	dc->desc = "PCI Hello World";
	/* qemu user things */
	dc->props = hello_properties;
	dc->reset = qdev_pci_hellodev_reset;
}


static const TypeINfo pci_hello_info = {
	.name = TYPE_PCI_HELLO_DEV,
	.parent = TYPE_PCI_DEVICE,
	.instance_size = sizeof(PCIHelloDevState),
	.class_init = pci_hellodev_class_init,
};


static void pci_hello_register_types(void)
{
	type_register_static(&pci_hello_info);
}

type_init(pci_hello_register_types);
