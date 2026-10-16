// AUTO-DRAFT from torvalds/linux PR #6a66840a68683d9361a9b31faf3bd8104428ad29
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
#define PCI_MAKE_VERSION(major, minor) ((u32)(((major) << 16) | (minor)))
/* …（同文件无关代码省略）… */
enum pci_protocol_version_t {
	PCI_PROTOCOL_VERSION_1_1 = PCI_MAKE_VERSION(1, 1),	/* Win10 */
	PCI_PROTOCOL_VERSION_1_2 = PCI_MAKE_VERSION(1, 2),	/* RS1 */
	PCI_PROTOCOL_VERSION_1_3 = PCI_MAKE_VERSION(1, 3),	/* Vibranium */
	PCI_PROTOCOL_VERSION_1_4 = PCI_MAKE_VERSION(1, 4),	/* WS2022 */
};
/* …（同文件无关代码省略）… */
#define PCI_CONFIG_MMIO_LENGTH	0x2000
#define CFG_PAGE_OFFSET 0x1000
#define CFG_PAGE_SIZE (PCI_CONFIG_MMIO_LENGTH - CFG_PAGE_OFFSET)
/* …（同文件无关代码省略）… */
#define STATUS_REVISION_MISMATCH 0xC0000059
/* …（同文件无关代码省略）… */
#define HV_PCI_RQSTOR_SIZE 64
/* …（同文件无关代码省略）… */
union win_slot_encoding {
	struct {
		u32	dev:5;
		u32	func:3;
		u32	reserved:24;
	} bits;
	u32 slot;
} __packed;
/* …（同文件无关代码省略）… */
struct pci_function_description {
	u16	v_id;	/* vendor ID */
	u16	d_id;	/* device ID */
	u8	rev;
	u8	prog_intf;
	u8	subclass;
	u8	base_class;
	u32	subsystem_id;
	union win_slot_encoding win_slot;
	u32	ser;	/* serial number */
} __packed;
/* …（同文件无关代码省略）… */
struct pci_function_description2 {
	u16	v_id;	/* vendor ID */
	u16	d_id;	/* device ID */
	u8	rev;
	u8	prog_intf;
	u8	subclass;
	u8	base_class;
	u32	subsystem_id;
	union	win_slot_encoding win_slot;
	u32	ser;	/* serial number */
	u32	flags;
	u16	virtual_numa_node;
	u16	reserved;
} __packed;
/* …（同文件无关代码省略）… */
struct tran_int_desc {
	u16	reserved;
	u16	vector_count;
	u32	data;
	u64	address;
} __packed;
/* …（同文件无关代码省略）… */
struct pci_message {
	u32 type;
} __packed;
/* …（同文件无关代码省略）… */
struct pci_child_message {
	struct pci_message message_type;
	union win_slot_encoding wslot;
} __packed;
/* …（同文件无关代码省略）… */
struct pci_incoming_message {
	struct vmpacket_descriptor hdr;
	struct pci_message message_type;
} __packed;
/* …（同文件无关代码省略）… */
struct pci_packet {
	void (*completion_func)(void *context, struct pci_response *resp,
				int resp_packet_size);
	void *compl_ctxt;
};
/* …（同文件无关代码省略）… */
struct pci_bus_d0_entry {
	struct pci_message message_type;
	u32 reserved;
	u64 mmio_base;
} __packed;
/* …（同文件无关代码省略）… */
struct pci_bus_relations {
	struct pci_incoming_message incoming;
	u32 device_count;
	struct pci_function_description func[];
} __packed;
/* …（同文件无关代码省略）… */
struct pci_bus_relations2 {
	struct pci_incoming_message incoming;
	u32 device_count;
	struct pci_function_description2 func[];
} __packed;
/* …（同文件无关代码省略）… */
struct pci_resources_assigned {
	struct pci_message message_type;
	union win_slot_encoding wslot;
	u8 memory_range[0x14][6];	/* not used here */
	u32 msi_descriptors;
	u32 reserved[4];
} __packed;
/* …（同文件无关代码省略）… */
struct pci_resources_assigned2 {
	struct pci_message message_type;
	union win_slot_encoding wslot;
	u8 memory_range[0x14][6];	/* not used here */
	u32 msi_descriptor_count;
	u8 reserved[70];
} __packed;
/* …（同文件无关代码省略）… */
struct pci_create_interrupt {
	struct pci_message message_type;
	union win_slot_encoding wslot;
	struct hv_msi_desc int_desc;
} __packed;
/* …（同文件无关代码省略）… */
struct pci_create_interrupt2 {
	struct pci_message message_type;
	union win_slot_encoding wslot;
	struct hv_msi_desc2 int_desc;
} __packed;
/* …（同文件无关代码省略）… */
struct pci_create_interrupt3 {
	struct pci_message message_type;
	union win_slot_encoding wslot;
	struct hv_msi_desc3 int_desc;
} __packed;
/* …（同文件无关代码省略）… */
struct pci_dev_inval_block {
	struct pci_incoming_message incoming;
	union win_slot_encoding wslot;
	u64 block_mask;
} __packed;
/* …（同文件无关代码省略）… */
struct pci_dev_incoming {
	struct pci_incoming_message incoming;
	union win_slot_encoding wslot;
} __packed;
/* …（同文件无关代码省略）… */
struct pci_eject_response {
	struct pci_message message_type;
	union win_slot_encoding wslot;
	u32 status;
} __packed;
/* …（同文件无关代码省略）… */
enum hv_pcibus_state {
	hv_pcibus_init = 0,
	hv_pcibus_probed,
	hv_pcibus_installed,
	hv_pcibus_removing,
	hv_pcibus_maximum
};
/* …（同文件无关代码省略）… */
struct hv_pcibus_device {
#ifdef CONFIG_X86
	struct pci_sysdata sysdata;
#elif defined(CONFIG_ARM64)
	struct pci_config_window sysdata;
#endif
	struct pci_host_bridge *bridge;
	struct fwnode_handle *fwnode;
	/* Protocol version negotiated with the host */
	enum pci_protocol_version_t protocol_version;

	struct mutex state_lock;
	enum hv_pcibus_state state;

	struct hv_device *hdev;
	resource_size_t low_mmio_space;
	resource_size_t high_mmio_space;
	struct resource *mem_config;
	struct resource *low_mmio_res;
	struct resource *high_mmio_res;
	struct completion *survey_event;
	spinlock_t config_lock;	/* Avoid two threads writing index page */
	spinlock_t device_list_lock;	/* Protect lists below */
	void __iomem *cfg_addr;

	struct list_head children;
	struct list_head dr_list;

	struct irq_domain *irq_domain;

	struct workqueue_struct *wq;

	/* Highest slot of child device with resources allocated */
	int wslot_res_allocated;
	bool use_calls; /* Use hypercalls to access mmio cfg space */
};
/* …（同文件无关代码省略）… */
struct hv_dr_work {
	struct work_struct wrk;
	struct hv_pcibus_device *bus;
};
/* …（同文件无关代码省略）… */
struct hv_pcidev_description {
	u16	v_id;	/* vendor ID */
	u16	d_id;	/* device ID */
	u8	rev;
	u8	prog_intf;
	u8	subclass;
	u8	base_class;
	u32	subsystem_id;
	union	win_slot_encoding win_slot;
	u32	ser;	/* serial number */
	u32	flags;
	u16	virtual_numa_node;
};
/* …（同文件无关代码省略）… */
struct hv_dr_state {
	struct list_head list_entry;
	u32 device_count;
	struct hv_pcidev_description func[] __counted_by(device_count);
};
/* …（同文件无关代码省略）… */
struct hv_pci_dev {
	/* List protected by pci_rescan_remove_lock */
	struct list_head list_entry;
	refcount_t refs;
	struct pci_slot *pci_slot;
	struct hv_pcidev_description desc;
	bool reported_missing;
	struct hv_pcibus_device *hbus;
	struct work_struct wrk;

	void (*block_invalidate)(void *context, u64 block_mask);
	void *invalidate_context;

	/*
	 * What would be observed if one wrote 0xFFFFFFFF to a BAR and then
	 * read it back, for each of the BAR offsets within config space.
	 */
	u32 probed_bar[PCI_STD_NUM_BARS];
};
/* …（同文件无关代码省略）… */
struct hv_pci_compl {
	struct completion host_event;
	s32 completion_status;
};
/* …（同文件无关代码省略）… */
static unsigned int hv_msi_get_int_vector(struct irq_data *data)
{
	struct irq_cfg *cfg = irqd_cfg(data);

	return cfg->vector;
}
/* …（同文件无关代码省略）… */
static void hv_pci_generic_compl(void *context, struct pci_response *resp,
				 int resp_packet_size)
{
	struct hv_pci_compl *comp_pkt = context;

	comp_pkt->completion_status = resp->status;
	complete(&comp_pkt->host_event);
}
/* …（同文件无关代码省略）… */
static void get_pcichild(struct hv_pci_dev *hpdev)
{
	refcount_inc(&hpdev->refs);
}
/* …（同文件无关代码省略）… */
static void put_pcichild(struct hv_pci_dev *hpdev)
{
	if (refcount_dec_and_test(&hpdev->refs))
		kfree(hpdev);
}

/*
 * There is no good way to get notified from vmbus_onoffer_rescind(),
 * so let's use polling here, since this is not a hot path.
 */
static int wait_for_response(struct hv_device *hdev,
			     struct completion *comp)
{
	while (true) {
		if (hdev->channel->rescind) {
			dev_warn_once(&hdev->device, "The device is gone.\n");
			return -ENODEV;
		}

		if (wait_for_completion_timeout(comp, HZ / 10))
			break;
	}

	return 0;
/* …（同文件无关代码省略）… */
static u32 devfn_to_wslot(int devfn)
{
	union win_slot_encoding wslot;

	wslot.slot = 0;
	wslot.bits.dev = PCI_SLOT(devfn);
	wslot.bits.func = PCI_FUNC(devfn);

	return wslot.slot;
}
/* …（同文件无关代码省略）… */
static int wslot_to_devfn(u32 wslot)
{
	union win_slot_encoding slot_no;

	slot_no.slot = wslot;
	return PCI_DEVFN(slot_no.bits.dev, slot_no.bits.func);
}
/* …（同文件无关代码省略）… */
static void hv_pci_read_mmio(struct device *dev, phys_addr_t gpa, int size, u32 *val)
{
	struct hv_mmio_read_input *in;
	struct hv_mmio_read_output *out;
	u64 ret;

	/*
	 * Must be called with interrupts disabled so it is safe
	 * to use the per-cpu input argument page.  Use it for
	 * both input and output.
	 */
	in = *this_cpu_ptr(hyperv_pcpu_input_arg);
	out = *this_cpu_ptr(hyperv_pcpu_input_arg) + sizeof(*in);
	in->gpa = gpa;
	in->size = size;

	ret = hv_do_hypercall(HVCALL_MMIO_READ, in, out);
	if (hv_result_success(ret)) {
		switch (size) {
		case 1:
			*val = *(u8 *)(out->data);
			break;
		case 2:
			*val = *(u16 *)(out->data);
			break;
		default:
			*val = *(u32 *)(out->data);
			break;
		}
	} else
		dev_err(dev, "MMIO read hypercall error %llx addr %llx size %d\n",
				ret, gpa, size);
}
/* …（同文件无关代码省略）… */
static void hv_pci_write_mmio(struct device *dev, phys_addr_t gpa, int size, u32 val)
{
	struct hv_mmio_write_input *in;
	u64 ret;

	/*
	 * Must be called with interrupts disabled so it is safe
	 * to use the per-cpu input argument memory.
	 */
	in = *this_cpu_ptr(hyperv_pcpu_input_arg);
	in->gpa = gpa;
	in->size = size;
	switch (size) {
	case 1:
		*(u8 *)(in->data) = val;
		break;
	case 2:
		*(u16 *)(in->data) = val;
		break;
	default:
		*(u32 *)(in->data) = val;
		break;
	}

	ret = hv_do_hypercall(HVCALL_MMIO_WRITE, in, NULL);
	if (!hv_result_success(ret))
		dev_err(dev, "MMIO write hypercall error %llx addr %llx size %d\n",
				ret, gpa, size);
}
/* …（同文件无关代码省略）… */
static void _hv_pcifront_read_config(struct hv_pci_dev *hpdev, int where,
				     int size, u32 *val)
{
	struct hv_pcibus_device *hbus = hpdev->hbus;
	struct device *dev = &hbus->hdev->device;
	int offset = where + CFG_PAGE_OFFSET;
	unsigned long flags;

	/*
	 * If the attempt is to read the IDs or the ROM BAR, simulate that.
	 */
	if (where + size <= PCI_COMMAND) {
		memcpy(val, ((u8 *)&hpdev->desc.v_id) + where, size);
	} else if (where >= PCI_CLASS_REVISION && where + size <=
		   PCI_CACHE_LINE_SIZE) {
		memcpy(val, ((u8 *)&hpdev->desc.rev) + where -
		       PCI_CLASS_REVISION, size);
	} else if (where >= PCI_SUBSYSTEM_VENDOR_ID && where + size <=
		   PCI_ROM_ADDRESS) {
		memcpy(val, (u8 *)&hpdev->desc.subsystem_id + where -
		       PCI_SUBSYSTEM_VENDOR_ID, size);
	} else if (where >= PCI_ROM_ADDRESS && where + size <=
		   PCI_CAPABILITY_LIST) {
		/* ROM BARs are unimplemented */
		*val = 0;
	} else if ((where >= PCI_INTERRUPT_LINE && where + size <= PCI_INTERRUPT_PIN) ||
		   (where >= PCI_INTERRUPT_PIN && where + size <= PCI_MIN_GNT)) {
		/*
		 * Interrupt Line and Interrupt PIN are hard-wired to zero
		 * because this front-end only supports message-signaled
		 * interrupts.
		 */
		*val = 0;
	} else if (where + size <= CFG_PAGE_SIZE) {

		spin_lock_irqsave(&hbus->config_lock, flags);
		if (hbus->use_calls) {
			phys_addr_t addr = hbus->mem_config->start + offset;

			hv_pci_write_mmio(dev, hbus->mem_config->start, 4,
						hpdev->desc.win_slot.slot);
			hv_pci_read_mmio(dev, addr, size, val);
		} else {
			void __iomem *addr = hbus->cfg_addr + offset;

			/* Choose the function to be read. (See comment above) */
			writel(hpdev->desc.win_slot.slot, hbus->cfg_addr);
			/* Make sure the function was chosen before reading. */
			mb();
			/* Read from that function's config space. */
			switch (size) {
			case 1:
				*val = readb(addr);
				break;
			case 2:
				*val = readw(addr);
				break;
			default:
				*val = readl(addr);
				break;
			}
			/*
			 * Make sure the read was done before we release the
			 * spinlock allowing consecutive reads/writes.
			 */
			mb();
		}
		spin_unlock_irqrestore(&hbus->config_lock, flags);
	} else {
		dev_err(dev, "Attempt to read beyond a function's config space.\n");
	}
}
/* …（同文件无关代码省略）… */
static u16 hv_pcifront_get_vendor_id(struct hv_pci_dev *hpdev)
{
	struct hv_pcibus_device *hbus = hpdev->hbus;
	struct device *dev = &hbus->hdev->device;
	u32 val;
	u16 ret;
	unsigned long flags;

	spin_lock_irqsave(&hbus->config_lock, flags);

	if (hbus->use_calls) {
		phys_addr_t addr = hbus->mem_config->start +
					 CFG_PAGE_OFFSET + PCI_VENDOR_ID;

		hv_pci_write_mmio(dev, hbus->mem_config->start, 4,
					hpdev->desc.win_slot.slot);
		hv_pci_read_mmio(dev, addr, 2, &val);
		ret = val;  /* Truncates to 16 bits */
	} else {
		void __iomem *addr = hbus->cfg_addr + CFG_PAGE_OFFSET +
					     PCI_VENDOR_ID;
		/* Choose the function to be read. (See comment above) */
		writel(hpdev->desc.win_slot.slot, hbus->cfg_addr);
		/* Make sure the function was chosen before we start reading. */
		mb();
		/* Read from that function's config space. */
		ret = readw(addr);
		/*
		 * mb() is not required here, because the
		 * spin_unlock_irqrestore() is a barrier.
		 */
	}

	spin_unlock_irqrestore(&hbus->config_lock, flags);

	return ret;
}
/* …（同文件无关代码省略）… */
static void _hv_pcifront_write_config(struct hv_pci_dev *hpdev, int where,
				      int size, u32 val)
{
	struct hv_pcibus_device *hbus = hpdev->hbus;
	struct device *dev = &hbus->hdev->device;
	int offset = where + CFG_PAGE_OFFSET;
	unsigned long flags;

	if (where >= PCI_SUBSYSTEM_VENDOR_ID &&
	    where + size <= PCI_CAPABILITY_LIST) {
		/* SSIDs and ROM BARs are read-only */
	} else if (where >= PCI_COMMAND && where + size <= CFG_PAGE_SIZE) {
		spin_lock_irqsave(&hbus->config_lock, flags);

		if (hbus->use_calls) {
			phys_addr_t addr = hbus->mem_config->start + offset;

			hv_pci_write_mmio(dev, hbus->mem_config->start, 4,
						hpdev->desc.win_slot.slot);
			hv_pci_write_mmio(dev, addr, size, val);
		} else {
			void __iomem *addr = hbus->cfg_addr + offset;

			/* Choose the function to write. (See comment above) */
			writel(hpdev->desc.win_slot.slot, hbus->cfg_addr);
			/* Make sure the function was chosen before writing. */
			wmb();
			/* Write to that function's config space. */
			switch (size) {
			case 1:
				writeb(val, addr);
				break;
			case 2:
				writew(val, addr);
				break;
			default:
				writel(val, addr);
				break;
			}
			/*
			 * Make sure the write was done before we release the
			 * spinlock allowing consecutive reads/writes.
			 */
			mb();
		}
		spin_unlock_irqrestore(&hbus->config_lock, flags);
	} else {
		dev_err(dev, "Attempt to write beyond a function's config space.\n");
	}
}
/* …（同文件无关代码省略）… */
	if (ret)
		return ret;

	ret = wait_for_response(hbus->hdev, &comp_pkt.comp_pkt.host_event);
	if (ret)
		return ret;

/* …（同文件无关代码省略）… */
	if (ret)
		return ret;

	ret = wait_for_response(hbus->hdev, &comp_pkt.host_event);
	if (ret)
		return ret;

/* …（同文件无关代码省略）… */
static void hv_int_desc_free(struct hv_pci_dev *hpdev,
			     struct tran_int_desc *int_desc)
{
	struct pci_delete_interrupt *int_pkt;
	struct {
		struct pci_packet pkt;
		u8 buffer[sizeof(struct pci_delete_interrupt)];
	} ctxt;

	if (!int_desc->vector_count) {
		kfree(int_desc);
		return;
	}
	memset(&ctxt, 0, sizeof(ctxt));
	int_pkt = (struct pci_delete_interrupt *)ctxt.buffer;
	int_pkt->message_type.type =
		PCI_DELETE_INTERRUPT_MESSAGE;
	int_pkt->wslot.slot = hpdev->desc.win_slot.slot;
	int_pkt->int_desc = *int_desc;
	vmbus_sendpacket(hpdev->hbus->hdev->channel, int_pkt, sizeof(*int_pkt),
			 0, VM_PKT_DATA_INBAND, 0);
	kfree(int_desc);
}
/* …（同文件无关代码省略）… */
struct compose_comp_ctxt {
	struct hv_pci_compl comp_pkt;
	struct tran_int_desc int_desc;
};
/* …（同文件无关代码省略）… */
static void hv_pci_compose_compl(void *context, struct pci_response *resp,
				 int resp_packet_size)
{
	struct compose_comp_ctxt *comp_pkt = context;
	struct pci_create_int_response *int_resp =
		(struct pci_create_int_response *)resp;

	if (resp_packet_size < sizeof(*int_resp)) {
		comp_pkt->comp_pkt.completion_status = -1;
		goto out;
	}
	comp_pkt->comp_pkt.completion_status = resp->status;
	comp_pkt->int_desc = int_resp->int_desc;
out:
	complete(&comp_pkt->comp_pkt.host_event);
}
/* …（同文件无关代码省略）… */
static u32 hv_compose_msi_req_v1(
	struct pci_create_interrupt *int_pkt,
	u32 slot, u8 vector, u16 vector_count)
{
	int_pkt->message_type.type = PCI_CREATE_INTERRUPT_MESSAGE;
	int_pkt->wslot.slot = slot;
	int_pkt->int_desc.vector = vector;
	int_pkt->int_desc.vector_count = vector_count;
	int_pkt->int_desc.delivery_mode = DELIVERY_MODE;

	/*
	 * Create MSI w/ dummy vCPU set, overwritten by subsequent retarget in
	 * hv_irq_unmask().
	 */
	int_pkt->int_desc.cpu_mask = CPU_AFFINITY_ALL;

	return sizeof(*int_pkt);
}
/* …（同文件无关代码省略）… */
static int hv_compose_msi_req_get_cpu(const struct cpumask *affinity)
{
	return cpumask_first_and(affinity, cpu_online_mask);
}
/* …（同文件无关代码省略）… */
static int hv_compose_multi_msi_req_get_cpu(void)
{
	static DEFINE_SPINLOCK(multi_msi_cpu_lock);

	/* -1 means starting with CPU 0 */
	static int cpu_next = -1;

	unsigned long flags;
	int cpu;

	spin_lock_irqsave(&multi_msi_cpu_lock, flags);

	cpu_next = cpumask_next_wrap(cpu_next, cpu_online_mask);
	cpu = cpu_next;

	spin_unlock_irqrestore(&multi_msi_cpu_lock, flags);

	return cpu;
}
/* …（同文件无关代码省略）… */
static u32 hv_compose_msi_req_v2(
	struct pci_create_interrupt2 *int_pkt, int cpu,
	u32 slot, u8 vector, u16 vector_count)
{
	int_pkt->message_type.type = PCI_CREATE_INTERRUPT_MESSAGE2;
	int_pkt->wslot.slot = slot;
	int_pkt->int_desc.vector = vector;
	int_pkt->int_desc.vector_count = vector_count;
	int_pkt->int_desc.delivery_mode = DELIVERY_MODE;
	int_pkt->int_desc.processor_array[0] =
		hv_cpu_number_to_vp_number(cpu);
	int_pkt->int_desc.processor_count = 1;

	return sizeof(*int_pkt);
}
/* …（同文件无关代码省略）… */
static u32 hv_compose_msi_req_v3(
	struct pci_create_interrupt3 *int_pkt, int cpu,
	u32 slot, u32 vector, u16 vector_count)
{
	int_pkt->message_type.type = PCI_CREATE_INTERRUPT_MESSAGE3;
	int_pkt->wslot.slot = slot;
	int_pkt->int_desc.vector = vector;
	int_pkt->int_desc.reserved = 0;
	int_pkt->int_desc.vector_count = vector_count;
	int_pkt->int_desc.delivery_mode = DELIVERY_MODE;
	int_pkt->int_desc.processor_array[0] =
		hv_cpu_number_to_vp_number(cpu);
	int_pkt->int_desc.processor_count = 1;

	return sizeof(*int_pkt);
}
/* …（同文件无关代码省略）… */
static void hv_compose_msi_msg(struct irq_data *data, struct msi_msg *msg)
{
	struct hv_pcibus_device *hbus;
	struct vmbus_channel *channel;
	struct hv_pci_dev *hpdev;
	struct pci_bus *pbus;
	struct pci_dev *pdev;
	const struct cpumask *dest;
	struct compose_comp_ctxt comp;
	struct tran_int_desc *int_desc;
	struct msi_desc *msi_desc;
	/*
	 * vector_count should be u16: see hv_msi_desc, hv_msi_desc2
	 * and hv_msi_desc3. vector must be u32: see hv_msi_desc3.
	 */
	u16 vector_count;
	u32 vector;
	struct {
		struct pci_packet pci_pkt;
		union {
			struct pci_create_interrupt v1;
			struct pci_create_interrupt2 v2;
			struct pci_create_interrupt3 v3;
		} int_pkts;
	} __packed ctxt;
	bool multi_msi;
	u64 trans_id;
	u32 size;
	int ret;
	int cpu;

	msi_desc  = irq_data_get_msi_desc(data);
	multi_msi = !msi_desc->pci.msi_attrib.is_msix &&
		    msi_desc->nvec_used > 1;

	/* Reuse the previous allocation */
	if (data->chip_data && multi_msi) {
		int_desc = data->chip_data;
		msg->address_hi = int_desc->address >> 32;
		msg->address_lo = int_desc->address & 0xffffffff;
		msg->data = int_desc->data;
		return;
	}

	pdev = msi_desc_to_pci_dev(msi_desc);
	dest = irq_data_get_effective_affinity_mask(data);
	pbus = pdev->bus;
	hbus = container_of(pbus->sysdata, struct hv_pcibus_device, sysdata);
	channel = hbus->hdev->channel;
	hpdev = get_pcichild_wslot(hbus, devfn_to_wslot(pdev->devfn));
	if (!hpdev)
		goto return_null_message;

	/* Free any previous message that might have already been composed. */
	if (data->chip_data && !multi_msi) {
		int_desc = data->chip_data;
		data->chip_data = NULL;
		hv_int_desc_free(hpdev, int_desc);
	}

	int_desc = kzalloc_obj(*int_desc, GFP_ATOMIC);
	if (!int_desc)
		goto drop_reference;

	if (multi_msi) {
		/*
		 * If this is not the first MSI of Multi MSI, we already have
		 * a mapping.  Can exit early.
		 */
		if (msi_desc->irq != data->irq) {
			data->chip_data = int_desc;
			int_desc->address = msi_desc->msg.address_lo |
					    (u64)msi_desc->msg.address_hi << 32;
			int_desc->data = msi_desc->msg.data +
					 (data->irq - msi_desc->irq);
			msg->address_hi = msi_desc->msg.address_hi;
			msg->address_lo = msi_desc->msg.address_lo;
			msg->data = int_desc->data;
			put_pcichild(hpdev);
			return;
		}
		/*
		 * The vector we select here is a dummy value.  The correct
		 * value gets sent to the hypervisor in unmask().  This needs
		 * to be aligned with the count, and also not zero.  Multi-msi
		 * is powers of 2 up to 32, so 32 will always work here.
		 */
		vector = 32;
		vector_count = msi_desc->nvec_used;
		cpu = hv_compose_multi_msi_req_get_cpu();
	} else {
		vector = hv_msi_get_int_vector(data);
		vector_count = 1;
		cpu = hv_compose_msi_req_get_cpu(dest);
	}

	/*
	 * hv_compose_msi_req_v1 and v2 are for x86 only, meaning 'vector'
	 * can't exceed u8. Cast 'vector' down to u8 for v1/v2 explicitly
	 * for better readability.
	 */
	memset(&ctxt, 0, sizeof(ctxt));
	init_completion(&comp.comp_pkt.host_event);
	ctxt.pci_pkt.completion_func = hv_pci_compose_compl;
	ctxt.pci_pkt.compl_ctxt = &comp;

	switch (hbus->protocol_version) {
	case PCI_PROTOCOL_VERSION_1_1:
		size = hv_compose_msi_req_v1(&ctxt.int_pkts.v1,
					hpdev->desc.win_slot.slot,
					(u8)vector,
					vector_count);
		break;

	case PCI_PROTOCOL_VERSION_1_2:
	case PCI_PROTOCOL_VERSION_1_3:
		size = hv_compose_msi_req_v2(&ctxt.int_pkts.v2,
					cpu,
					hpdev->desc.win_slot.slot,
					(u8)vector,
					vector_count);
		break;

	case PCI_PROTOCOL_VERSION_1_4:
		size = hv_compose_msi_req_v3(&ctxt.int_pkts.v3,
					cpu,
					hpdev->desc.win_slot.slot,
					vector,
					vector_count);
		break;

	default:
		/* As we only negotiate protocol versions known to this driver,
		 * this path should never hit. However, this is it not a hot
		 * path so we print a message to aid future updates.
		 */
		dev_err(&hbus->hdev->device,
			"Unexpected vPCI protocol, update driver.");
		goto free_int_desc;
	}

	ret = vmbus_sendpacket_getid(hpdev->hbus->hdev->channel, &ctxt.int_pkts,
				     size, (unsigned long)&ctxt.pci_pkt,
				     &trans_id, VM_PKT_DATA_INBAND,
				     VMBUS_DATA_PACKET_FLAG_COMPLETION_REQUESTED);
	if (ret) {
		dev_err(&hbus->hdev->device,
			"Sending request for interrupt failed: 0x%x",
			comp.comp_pkt.completion_status);
		goto free_int_desc;
	}

	/*
	 * Prevents hv_pci_onchannelcallback() from running concurrently
	 * in the tasklet.
	 */
	tasklet_disable_in_atomic(&channel->callback_event);

	/*
	 * Since this function is called with IRQ locks held, can't
	 * do normal wait for completion; instead poll.
	 */
	while (!try_wait_for_completion(&comp.comp_pkt.host_event)) {
		unsigned long flags;

		/* 0xFFFF means an invalid PCI VENDOR ID. */
		if (hv_pcifront_get_vendor_id(hpdev) == 0xFFFF) {
			dev_err_once(&hbus->hdev->device,
				     "the device has gone\n");
			goto enable_tasklet;
		}

		/*
		 * Make sure that the ring buffer data structure doesn't get
		 * freed while we dereference the ring buffer pointer.  Test
		 * for the channel's onchannel_callback being NULL within a
		 * sched_lock critical section.  See also the inline comments
		 * in vmbus_reset_channel_cb().
		 */
		spin_lock_irqsave(&channel->sched_lock, flags);
		if (unlikely(channel->onchannel_callback == NULL)) {
			spin_unlock_irqrestore(&channel->sched_lock, flags);
			goto enable_tasklet;
		}
		hv_pci_onchannelcallback(hbus);
		spin_unlock_irqrestore(&channel->sched_lock, flags);

		udelay(100);
	}

	tasklet_enable(&channel->callback_event);

	if (comp.comp_pkt.completion_status < 0) {
		dev_err(&hbus->hdev->device,
			"Request for interrupt failed: 0x%x",
			comp.comp_pkt.completion_status);
		goto free_int_desc;
	}

	/*
	 * Record the assignment so that this can be unwound later. Using
	 * irq_set_chip_data() here would be appropriate, but the lock it takes
	 * is already held.
	 */
	*int_desc = comp.int_desc;
	data->chip_data = int_desc;

	/* Pass up the result. */
	msg->address_hi = comp.int_desc.address >> 32;
	msg->address_lo = comp.int_desc.address & 0xffffffff;
	msg->data = comp.int_desc.data;

	put_pcichild(hpdev);
	return;

enable_tasklet:
	tasklet_enable(&channel->callback_event);
	/*
	 * The completion packet on the stack becomes invalid after 'return';
	 * remove the ID from the VMbus requestor if the identifier is still
	 * mapped to/associated with the packet.  (The identifier could have
	 * been 're-used', i.e., already removed and (re-)mapped.)
	 *
	 * Cf. hv_pci_onchannelcallback().
	 */
	vmbus_request_addr_match(channel, trans_id, (unsigned long)&ctxt.pci_pkt);
free_int_desc:
	kfree(int_desc);
drop_reference:
	put_pcichild(hpdev);
return_null_message:
	msg->address_hi = 0;
	msg->address_lo = 0;
	msg->data = 0;
}
/* …（同文件无关代码省略）… */
static u64 get_bar_size(u64 bar_val)
{
	return round_up((1 + ~(bar_val & PCI_BASE_ADDRESS_MEM_MASK)),
			PAGE_SIZE);
}
/* …（同文件无关代码省略）… */
static void prepopulate_bars(struct hv_pcibus_device *hbus)
{
	resource_size_t high_size = 0;
	resource_size_t low_size = 0;
	resource_size_t high_base = 0;
	resource_size_t low_base = 0;
	resource_size_t bar_size;
	struct hv_pci_dev *hpdev;
	unsigned long flags;
	u64 bar_val;
	u32 command;
	bool high;
	int i;

	if (hbus->low_mmio_space) {
		low_size = 1ULL << (63 - __builtin_clzll(hbus->low_mmio_space));
		low_base = hbus->low_mmio_res->start;
	}

	if (hbus->high_mmio_space) {
		high_size = 1ULL <<
			(63 - __builtin_clzll(hbus->high_mmio_space));
		high_base = hbus->high_mmio_res->start;
	}

	spin_lock_irqsave(&hbus->device_list_lock, flags);

	/*
	 * Clear the memory enable bit, in case it's already set. This occurs
	 * in the suspend path of hibernation, where the device is suspended,
	 * resumed and suspended again: see hibernation_snapshot() and
	 * hibernation_platform_enter().
	 *
	 * If the memory enable bit is already set, Hyper-V silently ignores
	 * the below BAR updates, and the related PCI device driver can not
	 * work, because reading from the device register(s) always returns
	 * 0xFFFFFFFF (PCI_ERROR_RESPONSE).
	 */
	list_for_each_entry(hpdev, &hbus->children, list_entry) {
		_hv_pcifront_read_config(hpdev, PCI_COMMAND, 2, &command);
		command &= ~PCI_COMMAND_MEMORY;
		_hv_pcifront_write_config(hpdev, PCI_COMMAND, 2, command);
	}

	/* Pick addresses for the BARs. */
	do {
		list_for_each_entry(hpdev, &hbus->children, list_entry) {
			for (i = 0; i < PCI_STD_NUM_BARS; i++) {
				bar_val = hpdev->probed_bar[i];
				if (bar_val == 0)
					continue;
				high = bar_val & PCI_BASE_ADDRESS_MEM_TYPE_64;
				if (high) {
					bar_val |=
						((u64)hpdev->probed_bar[i + 1]
						 << 32);
				} else {
					bar_val |= 0xffffffffULL << 32;
				}
				bar_size = get_bar_size(bar_val);
				if (high) {
					if (high_size != bar_size) {
						i++;
						continue;
					}
					_hv_pcifront_write_config(hpdev,
						PCI_BASE_ADDRESS_0 + (4 * i),
						4,
						(u32)(high_base & 0xffffff00));
					i++;
					_hv_pcifront_write_config(hpdev,
						PCI_BASE_ADDRESS_0 + (4 * i),
						4, (u32)(high_base >> 32));
					high_base += bar_size;
				} else {
					if (low_size != bar_size)
						continue;
					_hv_pcifront_write_config(hpdev,
						PCI_BASE_ADDRESS_0 + (4 * i),
						4,
						(u32)(low_base & 0xffffff00));
					low_base += bar_size;
				}
			}
			if (high_size <= 1 && low_size <= 1) {
				/*
				 * No need to set the PCI_COMMAND_MEMORY bit as
				 * the core PCI driver doesn't require the bit
				 * to be pre-set. Actually here we intentionally
				 * keep the bit off so that the PCI BAR probing
				 * in the core PCI driver doesn't cause Hyper-V
				 * to unnecessarily unmap/map the virtual BARs
				 * from/to the physical BARs multiple times.
				 * This reduces the VM boot time significantly
				 * if the BAR sizes are huge.
				 */
				break;
			}
		}

		high_size >>= 1;
		low_size >>= 1;
	}  while (high_size || low_size);

	spin_unlock_irqrestore(&hbus->device_list_lock, flags);
}
/* …（同文件无关代码省略）… */
static void hv_pci_remove_slots(struct hv_pcibus_device *hbus)
{
	struct hv_pci_dev *hpdev;

	list_for_each_entry(hpdev, &hbus->children, list_entry) {
		if (!hpdev->pci_slot)
			continue;
		pci_destroy_slot(hpdev->pci_slot);
		hpdev->pci_slot = NULL;
	}
}
/* …（同文件无关代码省略）… */
	if (ret)
		goto error;

	if (wait_for_response(hbus->hdev, &comp_pkt.host_event))
		goto error;

	hpdev->desc = *desc;
/* …（同文件无关代码省略）… */
static struct hv_pci_dev *get_pcichild_wslot(struct hv_pcibus_device *hbus,
					     u32 wslot)
{
	unsigned long flags;
	struct hv_pci_dev *iter, *hpdev = NULL;

	spin_lock_irqsave(&hbus->device_list_lock, flags);
	list_for_each_entry(iter, &hbus->children, list_entry) {
		if (iter->desc.win_slot.slot == wslot) {
			hpdev = iter;
			get_pcichild(hpdev);
			break;
		}
	}
	spin_unlock_irqrestore(&hbus->device_list_lock, flags);

	return hpdev;
}
/* …（同文件无关代码省略）… */
static void pci_devices_present_work(struct work_struct *work)
{
	u32 child_no;
	bool found;
	struct hv_pcidev_description *new_desc;
	struct hv_pci_dev *hpdev;
	struct hv_pcibus_device *hbus;
	struct list_head removed;
	struct hv_dr_work *dr_wrk;
	struct hv_dr_state *dr = NULL;
	unsigned long flags;

	dr_wrk = container_of(work, struct hv_dr_work, wrk);
	hbus = dr_wrk->bus;
	kfree(dr_wrk);

	INIT_LIST_HEAD(&removed);

	/* Pull this off the queue and process it if it was the last one. */
	spin_lock_irqsave(&hbus->device_list_lock, flags);
	while (!list_empty(&hbus->dr_list)) {
		dr = list_first_entry(&hbus->dr_list, struct hv_dr_state,
				      list_entry);
		list_del(&dr->list_entry);

		/* Throw this away if the list still has stuff in it. */
		if (!list_empty(&hbus->dr_list)) {
			kfree(dr);
			continue;
		}
	}
	spin_unlock_irqrestore(&hbus->device_list_lock, flags);

	if (!dr)
		return;

	mutex_lock(&hbus->state_lock);

	/* First, mark all existing children as reported missing. */
	spin_lock_irqsave(&hbus->device_list_lock, flags);
	list_for_each_entry(hpdev, &hbus->children, list_entry) {
		hpdev->reported_missing = true;
	}
	spin_unlock_irqrestore(&hbus->device_list_lock, flags);

	/* Next, add back any reported devices. */
	for (child_no = 0; child_no < dr->device_count; child_no++) {
		found = false;
		new_desc = &dr->func[child_no];

		spin_lock_irqsave(&hbus->device_list_lock, flags);
		list_for_each_entry(hpdev, &hbus->children, list_entry) {
			if ((hpdev->desc.win_slot.slot == new_desc->win_slot.slot) &&
			    (hpdev->desc.v_id == new_desc->v_id) &&
			    (hpdev->desc.d_id == new_desc->d_id) &&
			    (hpdev->desc.ser == new_desc->ser)) {
				hpdev->reported_missing = false;
				found = true;
			}
		}
		spin_unlock_irqrestore(&hbus->device_list_lock, flags);

		if (!found) {
			hpdev = new_pcichild_device(hbus, new_desc);
			if (!hpdev)
				dev_err(&hbus->hdev->device,
					"couldn't record a child device.\n");
		}
	}

	/* Move missing children to a list on the stack. */
	spin_lock_irqsave(&hbus->device_list_lock, flags);
	do {
		found = false;
		list_for_each_entry(hpdev, &hbus->children, list_entry) {
			if (hpdev->reported_missing) {
				found = true;
				put_pcichild(hpdev);
				list_move_tail(&hpdev->list_entry, &removed);
				break;
			}
		}
	} while (found);
	spin_unlock_irqrestore(&hbus->device_list_lock, flags);

	/* Delete everything that should no longer exist. */
	while (!list_empty(&removed)) {
		hpdev = list_first_entry(&removed, struct hv_pci_dev,
					 list_entry);
		list_del(&hpdev->list_entry);

		if (hpdev->pci_slot)
			pci_destroy_slot(hpdev->pci_slot);

		put_pcichild(hpdev);
	}

	switch (hbus->state) {
	case hv_pcibus_installed:
		/*
		 * Tell the core to rescan bus
		 * because there may have been changes.
		 */
		pci_lock_rescan_remove();
		pci_scan_child_bus(hbus->bridge->bus);
		hv_pci_assign_numa_node(hbus);
		hv_pci_assign_slots(hbus);
		pci_unlock_rescan_remove();
		break;

	case hv_pcibus_init:
	case hv_pcibus_probed:
		survey_child_resources(hbus);
		break;

	default:
		break;
	}

	mutex_unlock(&hbus->state_lock);

	kfree(dr);
}
/* …（同文件无关代码省略）… */
static int hv_pci_start_relations_work(struct hv_pcibus_device *hbus,
				       struct hv_dr_state *dr)
{
	struct hv_dr_work *dr_wrk;
	unsigned long flags;
	bool pending_dr;

	if (hbus->state == hv_pcibus_removing) {
		dev_info(&hbus->hdev->device,
			 "PCI VMBus BUS_RELATIONS: ignored\n");
		return -ENOENT;
	}

	dr_wrk = kzalloc_obj(*dr_wrk, GFP_NOWAIT);
	if (!dr_wrk)
		return -ENOMEM;

	INIT_WORK(&dr_wrk->wrk, pci_devices_present_work);
	dr_wrk->bus = hbus;

	spin_lock_irqsave(&hbus->device_list_lock, flags);
	/*
	 * If pending_dr is true, we have already queued a work,
	 * which will see the new dr. Otherwise, we need to
	 * queue a new work.
	 */
	pending_dr = !list_empty(&hbus->dr_list);
	list_add_tail(&dr->list_entry, &hbus->dr_list);
	spin_unlock_irqrestore(&hbus->device_list_lock, flags);

	if (pending_dr)
		kfree(dr_wrk);
	else
		queue_work(hbus->wq, &dr_wrk->wrk);

	return 0;
}
/* …（同文件无关代码省略）… */
static void hv_pci_devices_present(struct hv_pcibus_device *hbus,
				   struct pci_bus_relations *relations)
{
	struct hv_dr_state *dr;
	int i;

	dr = kzalloc_flex(*dr, func, relations->device_count, GFP_NOWAIT);
	if (!dr)
		return;

	dr->device_count = relations->device_count;
	for (i = 0; i < dr->device_count; i++) {
		dr->func[i].v_id = relations->func[i].v_id;
		dr->func[i].d_id = relations->func[i].d_id;
		dr->func[i].rev = relations->func[i].rev;
		dr->func[i].prog_intf = relations->func[i].prog_intf;
		dr->func[i].subclass = relations->func[i].subclass;
		dr->func[i].base_class = relations->func[i].base_class;
		dr->func[i].subsystem_id = relations->func[i].subsystem_id;
		dr->func[i].win_slot = relations->func[i].win_slot;
		dr->func[i].ser = relations->func[i].ser;
	}

	if (hv_pci_start_relations_work(hbus, dr))
		kfree(dr);
}
/* …（同文件无关代码省略）… */
static void hv_pci_devices_present2(struct hv_pcibus_device *hbus,
				    struct pci_bus_relations2 *relations)
{
	struct hv_dr_state *dr;
	int i;

	dr = kzalloc_flex(*dr, func, relations->device_count, GFP_NOWAIT);
	if (!dr)
		return;

	dr->device_count = relations->device_count;
	for (i = 0; i < dr->device_count; i++) {
		dr->func[i].v_id = relations->func[i].v_id;
		dr->func[i].d_id = relations->func[i].d_id;
		dr->func[i].rev = relations->func[i].rev;
		dr->func[i].prog_intf = relations->func[i].prog_intf;
		dr->func[i].subclass = relations->func[i].subclass;
		dr->func[i].base_class = relations->func[i].base_class;
		dr->func[i].subsystem_id = relations->func[i].subsystem_id;
		dr->func[i].win_slot = relations->func[i].win_slot;
		dr->func[i].ser = relations->func[i].ser;
		dr->func[i].flags = relations->func[i].flags;
		dr->func[i].virtual_numa_node =
			relations->func[i].virtual_numa_node;
	}

	if (hv_pci_start_relations_work(hbus, dr))
		kfree(dr);
}
/* …（同文件无关代码省略）… */
static void hv_eject_device_work(struct work_struct *work)
{
	struct pci_eject_response *ejct_pkt;
	struct hv_pcibus_device *hbus;
	struct hv_pci_dev *hpdev;
	struct pci_dev *pdev;
	unsigned long flags;
	int wslot;
	struct {
		struct pci_packet pkt;
		u8 buffer[sizeof(struct pci_eject_response)];
	} ctxt;

	hpdev = container_of(work, struct hv_pci_dev, wrk);
	hbus = hpdev->hbus;

	mutex_lock(&hbus->state_lock);

	/*
	 * Ejection can come before or after the PCI bus has been set up, so
	 * attempt to find it and tear down the bus state, if it exists.  This
	 * must be done without constructs like pci_domain_nr(hbus->bridge->bus)
	 * because hbus->bridge->bus may not exist yet.
	 */
	wslot = wslot_to_devfn(hpdev->desc.win_slot.slot);
	pdev = pci_get_domain_bus_and_slot(hbus->bridge->domain_nr, 0, wslot);
	if (pdev) {
		pci_lock_rescan_remove();
		pci_stop_and_remove_bus_device(pdev);
		pci_dev_put(pdev);
		pci_unlock_rescan_remove();
	}

	spin_lock_irqsave(&hbus->device_list_lock, flags);
	list_del(&hpdev->list_entry);
	spin_unlock_irqrestore(&hbus->device_list_lock, flags);

	if (hpdev->pci_slot)
		pci_destroy_slot(hpdev->pci_slot);

	memset(&ctxt, 0, sizeof(ctxt));
	ejct_pkt = (struct pci_eject_response *)ctxt.buffer;
	ejct_pkt->message_type.type = PCI_EJECTION_COMPLETE;
	ejct_pkt->wslot.slot = hpdev->desc.win_slot.slot;
	vmbus_sendpacket(hbus->hdev->channel, ejct_pkt,
			 sizeof(*ejct_pkt), 0,
			 VM_PKT_DATA_INBAND, 0);

	/* For the get_pcichild() in hv_pci_eject_device() */
	put_pcichild(hpdev);
	/* For the two refs got in new_pcichild_device() */
	put_pcichild(hpdev);
	put_pcichild(hpdev);
	/* hpdev has been freed. Do not use it any more. */

	mutex_unlock(&hbus->state_lock);
}
/* …（同文件无关代码省略）… */
static void hv_pci_eject_device(struct hv_pci_dev *hpdev)
{
	struct hv_pcibus_device *hbus = hpdev->hbus;
	struct hv_device *hdev = hbus->hdev;

	if (hbus->state == hv_pcibus_removing) {
		dev_info(&hdev->device, "PCI VMBus EJECT: ignored\n");
		return;
	}

	get_pcichild(hpdev);
	INIT_WORK(&hpdev->wrk, hv_eject_device_work);
	queue_work(hbus->wq, &hpdev->wrk);
}
/* …（同文件无关代码省略）… */
static void hv_pci_onchannelcallback(void *context)
{
	const int packet_size = 0x100;
	int ret;
	struct hv_pcibus_device *hbus = context;
	struct vmbus_channel *chan = hbus->hdev->channel;
	u32 bytes_recvd;
	u64 req_id, req_addr;
	struct vmpacket_descriptor *desc;
	unsigned char *buffer;
	int bufferlen = packet_size;
	struct pci_packet *comp_packet;
	struct pci_response *response;
	struct pci_incoming_message *new_message;
	struct pci_bus_relations *bus_rel;
	struct pci_bus_relations2 *bus_rel2;
	struct pci_dev_inval_block *inval;
	struct pci_dev_incoming *dev_message;
	struct hv_pci_dev *hpdev;
	unsigned long flags;

	buffer = kmalloc(bufferlen, GFP_ATOMIC);
	if (!buffer)
		return;

	while (1) {
		ret = vmbus_recvpacket_raw(chan, buffer, bufferlen,
					   &bytes_recvd, &req_id);

		if (ret == -ENOBUFS) {
			kfree(buffer);
			/* Handle large packet */
			bufferlen = bytes_recvd;
			buffer = kmalloc(bytes_recvd, GFP_ATOMIC);
			if (!buffer)
				return;
			continue;
		}

		/* Zero length indicates there are no more packets. */
		if (ret || !bytes_recvd)
			break;

		/*
		 * All incoming packets must be at least as large as a
		 * response.
		 */
		if (bytes_recvd <= sizeof(struct pci_response))
			continue;
		desc = (struct vmpacket_descriptor *)buffer;

		switch (desc->type) {
		case VM_PKT_COMP:

			lock_requestor(chan, flags);
			req_addr = __vmbus_request_addr_match(chan, req_id,
							      VMBUS_RQST_ADDR_ANY);
			if (req_addr == VMBUS_RQST_ERROR) {
				unlock_requestor(chan, flags);
				dev_err(&hbus->hdev->device,
					"Invalid transaction ID %llx\n",
					req_id);
				break;
			}
			comp_packet = (struct pci_packet *)req_addr;
			response = (struct pci_response *)buffer;
			/*
			 * Call ->completion_func() within the critical section to make
			 * sure that the packet pointer is still valid during the call:
			 * here 'valid' means that there's a task still waiting for the
			 * completion, and that the packet data is still on the waiting
			 * task's stack.  Cf. hv_compose_msi_msg().
			 */
			comp_packet->completion_func(comp_packet->compl_ctxt,
						     response,
						     bytes_recvd);
			unlock_requestor(chan, flags);
			break;

		case VM_PKT_DATA_INBAND:

			new_message = (struct pci_incoming_message *)buffer;
			switch (new_message->message_type.type) {
			case PCI_BUS_RELATIONS:

				bus_rel = (struct pci_bus_relations *)buffer;
				if (bytes_recvd < sizeof(*bus_rel) ||
				    bytes_recvd <
					struct_size(bus_rel, func,
						    bus_rel->device_count)) {
					dev_err(&hbus->hdev->device,
						"bus relations too small\n");
					break;
				}

				hv_pci_devices_present(hbus, bus_rel);
				break;

			case PCI_BUS_RELATIONS2:

				bus_rel2 = (struct pci_bus_relations2 *)buffer;
				if (bytes_recvd < sizeof(*bus_rel2) ||
				    bytes_recvd <
					struct_size(bus_rel2, func,
						    bus_rel2->device_count)) {
					dev_err(&hbus->hdev->device,
						"bus relations v2 too small\n");
					break;
				}

				hv_pci_devices_present2(hbus, bus_rel2);
				break;

			case PCI_EJECT:

				dev_message = (struct pci_dev_incoming *)buffer;
				if (bytes_recvd < sizeof(*dev_message)) {
					dev_err(&hbus->hdev->device,
						"eject message too small\n");
					break;
				}
				hpdev = get_pcichild_wslot(hbus,
						      dev_message->wslot.slot);
				if (hpdev) {
					hv_pci_eject_device(hpdev);
					put_pcichild(hpdev);
				}
				break;

			case PCI_INVALIDATE_BLOCK:

				inval = (struct pci_dev_inval_block *)buffer;
				if (bytes_recvd < sizeof(*inval)) {
					dev_err(&hbus->hdev->device,
						"invalidate message too small\n");
					break;
				}
				hpdev = get_pcichild_wslot(hbus,
							   inval->wslot.slot);
				if (hpdev) {
					if (hpdev->block_invalidate) {
						hpdev->block_invalidate(
						    hpdev->invalidate_context,
						    inval->block_mask);
					}
					put_pcichild(hpdev);
				}
				break;

			default:
				dev_warn(&hbus->hdev->device,
					"Unimplemented protocol message %x\n",
					new_message->message_type.type);
				break;
			}
			break;

		default:
			dev_err(&hbus->hdev->device,
				"unhandled packet type %d, tid %llx len %d\n",
				desc->type, req_id, bytes_recvd);
			break;
		}
	}

	kfree(buffer);
}
/* …（同文件无关代码省略）… */
static int hv_pci_protocol_negotiation(struct hv_device *hdev,
				       enum pci_protocol_version_t version[],
				       int num_version)
{
	struct hv_pcibus_device *hbus = hv_get_drvdata(hdev);
	struct pci_version_request *version_req;
	struct hv_pci_compl comp_pkt;
	struct pci_packet *pkt;
	int ret;
	int i;

	/*
	 * Initiate the handshake with the host and negotiate
	 * a version that the host can support. We start with the
	 * highest version number and go down if the host cannot
	 * support it.
	 */
	pkt = kzalloc(sizeof(*pkt) + sizeof(*version_req), GFP_KERNEL);
	if (!pkt)
		return -ENOMEM;

	init_completion(&comp_pkt.host_event);
	pkt->completion_func = hv_pci_generic_compl;
	pkt->compl_ctxt = &comp_pkt;
	version_req = (struct pci_version_request *)(pkt + 1);
	version_req->message_type.type = PCI_QUERY_PROTOCOL_VERSION;

	for (i = 0; i < num_version; i++) {
		version_req->protocol_version = version[i];
		ret = vmbus_sendpacket(hdev->channel, version_req,
				sizeof(struct pci_version_request),
				(unsigned long)pkt, VM_PKT_DATA_INBAND,
				VMBUS_DATA_PACKET_FLAG_COMPLETION_REQUESTED);
		if (!ret)
			ret = wait_for_response(hdev, &comp_pkt.host_event);

		if (ret) {
			dev_err(&hdev->device,
				"PCI Pass-through VSP failed to request version: %d",
				ret);
			goto exit;
		}

		if (comp_pkt.completion_status >= 0) {
			hbus->protocol_version = version[i];
			dev_info(&hdev->device,
				"PCI VMBus probing: Using version %#x\n",
				hbus->protocol_version);
			goto exit;
		}

		if (comp_pkt.completion_status != STATUS_REVISION_MISMATCH) {
			dev_err(&hdev->device,
				"PCI Pass-through VSP failed version request: %#x",
				comp_pkt.completion_status);
			ret = -EPROTO;
			goto exit;
		}

		reinit_completion(&comp_pkt.host_event);
	}

	dev_err(&hdev->device,
		"PCI pass-through VSP failed to find supported version");
	ret = -EPROTO;

exit:
	kfree(pkt);
	return ret;
}
/* …（同文件无关代码省略）… */
static void hv_pci_free_bridge_windows(struct hv_pcibus_device *hbus)
{
	/*
	 * Set the resources back to the way they looked when they
	 * were allocated by setting IORESOURCE_BUSY again.
	 */

	if (hbus->low_mmio_space && hbus->low_mmio_res) {
		hbus->low_mmio_res->flags |= IORESOURCE_BUSY;
		vmbus_free_mmio(hbus->low_mmio_res->start,
				resource_size(hbus->low_mmio_res));
	}

	if (hbus->high_mmio_space && hbus->high_mmio_res) {
		hbus->high_mmio_res->flags |= IORESOURCE_BUSY;
		vmbus_free_mmio(hbus->high_mmio_res->start,
				resource_size(hbus->high_mmio_res));
	}
}
/* …（同文件无关代码省略）… */
static void hv_free_config_window(struct hv_pcibus_device *hbus)
{
	vmbus_free_mmio(hbus->mem_config->start, PCI_CONFIG_MMIO_LENGTH);
}
/* …（同文件无关代码省略）… */
static int hv_pci_enter_d0(struct hv_device *hdev)
{
	struct hv_pcibus_device *hbus = hv_get_drvdata(hdev);
	struct pci_bus_d0_entry *d0_entry;
	struct hv_pci_compl comp_pkt;
	struct pci_packet *pkt;
	bool retry = true;
	int ret;

enter_d0_retry:
	/*
	 * Tell the host that the bus is ready to use, and moved into the
	 * powered-on state.  This includes telling the host which region
	 * of memory-mapped I/O space has been chosen for configuration space
	 * access.
	 */
	pkt = kzalloc(sizeof(*pkt) + sizeof(*d0_entry), GFP_KERNEL);
	if (!pkt)
		return -ENOMEM;

	init_completion(&comp_pkt.host_event);
	pkt->completion_func = hv_pci_generic_compl;
	pkt->compl_ctxt = &comp_pkt;
	d0_entry = (struct pci_bus_d0_entry *)(pkt + 1);
	d0_entry->message_type.type = PCI_BUS_D0ENTRY;
	d0_entry->mmio_base = hbus->mem_config->start;

	ret = vmbus_sendpacket(hdev->channel, d0_entry, sizeof(*d0_entry),
			       (unsigned long)pkt, VM_PKT_DATA_INBAND,
			       VMBUS_DATA_PACKET_FLAG_COMPLETION_REQUESTED);
	if (!ret)
		ret = wait_for_response(hdev, &comp_pkt.host_event);

	if (ret)
		goto exit;

	/*
	 * In certain case (Kdump) the pci device of interest was
	 * not cleanly shut down and resource is still held on host
	 * side, the host could return invalid device status.
	 * We need to explicitly request host to release the resource
	 * and try to enter D0 again.
	 */
	if (comp_pkt.completion_status < 0 && retry) {
		retry = false;

		dev_err(&hdev->device, "Retrying D0 Entry\n");

		/*
		 * Hv_pci_bus_exit() calls hv_send_resource_released()
		 * to free up resources of its child devices.
		 * In the kdump kernel we need to set the
		 * wslot_res_allocated to 255 so it scans all child
		 * devices to release resources allocated in the
		 * normal kernel before panic happened.
		 */
		hbus->wslot_res_allocated = 255;

		ret = hv_pci_bus_exit(hdev, true);

		if (ret == 0) {
			kfree(pkt);
			goto enter_d0_retry;
		}
		dev_err(&hdev->device,
			"Retrying D0 failed with ret %d\n", ret);
	}

	if (comp_pkt.completion_status < 0) {
		dev_err(&hdev->device,
			"PCI Pass-through VSP failed D0 Entry with status %x\n",
			comp_pkt.completion_status);
		ret = -EPROTO;
		goto exit;
	}

	ret = 0;

exit:
	kfree(pkt);
	return ret;
}
/* …（同文件无关代码省略）… */
static int hv_pci_query_relations(struct hv_device *hdev)
{
	struct hv_pcibus_device *hbus = hv_get_drvdata(hdev);
	struct pci_message message;
	struct completion comp;
	int ret;

	/* Ask the host to send along the list of child devices */
	init_completion(&comp);
	if (cmpxchg(&hbus->survey_event, NULL, &comp))
		return -ENOTEMPTY;

	memset(&message, 0, sizeof(message));
	message.type = PCI_QUERY_BUS_RELATIONS;

	ret = vmbus_sendpacket(hdev->channel, &message, sizeof(message),
			       0, VM_PKT_DATA_INBAND, 0);
	if (!ret)
		ret = wait_for_response(hdev, &comp);

	/*
	 * In the case of fast device addition/removal, it's possible that
	 * vmbus_sendpacket() or wait_for_response() returns -ENODEV but we
	 * already got a PCI_BUS_RELATIONS* message from the host and the
	 * channel callback already scheduled a work to hbus->wq, which can be
	 * running pci_devices_present_work() -> survey_child_resources() ->
	 * complete(&hbus->survey_event), even after hv_pci_query_relations()
	 * exits and the stack variable 'comp' is no longer valid; as a result,
	 * a hang or a page fault may happen when the complete() calls
	 * raw_spin_lock_irqsave(). Flush hbus->wq before we exit from
	 * hv_pci_query_relations() to avoid the issues. Note: if 'ret' is
	 * -ENODEV, there can't be any more work item scheduled to hbus->wq
	 * after the flush_workqueue(): see vmbus_onoffer_rescind() ->
	 * vmbus_reset_channel_cb(), vmbus_rescind_cleanup() ->
	 * channel->rescind = true.
	 */
	flush_workqueue(hbus->wq);

	return ret;
}
/* …（同文件无关代码省略）… */
static int hv_send_resources_allocated(struct hv_device *hdev)
{
	struct hv_pcibus_device *hbus = hv_get_drvdata(hdev);
	struct pci_resources_assigned *res_assigned;
	struct pci_resources_assigned2 *res_assigned2;
	struct hv_pci_compl comp_pkt;
	struct hv_pci_dev *hpdev;
	struct pci_packet *pkt;
	size_t size_res;
	int wslot;
	int ret;

	size_res = (hbus->protocol_version < PCI_PROTOCOL_VERSION_1_2)
			? sizeof(*res_assigned) : sizeof(*res_assigned2);

	pkt = kmalloc(sizeof(*pkt) + size_res, GFP_KERNEL);
	if (!pkt)
		return -ENOMEM;

	ret = 0;

	for (wslot = 0; wslot < 256; wslot++) {
		hpdev = get_pcichild_wslot(hbus, wslot);
		if (!hpdev)
			continue;

		memset(pkt, 0, sizeof(*pkt) + size_res);
		init_completion(&comp_pkt.host_event);
		pkt->completion_func = hv_pci_generic_compl;
		pkt->compl_ctxt = &comp_pkt;

		if (hbus->protocol_version < PCI_PROTOCOL_VERSION_1_2) {
			res_assigned =
				(struct pci_resources_assigned *)(pkt + 1);
			res_assigned->message_type.type =
				PCI_RESOURCES_ASSIGNED;
			res_assigned->wslot.slot = hpdev->desc.win_slot.slot;
		} else {
			res_assigned2 =
				(struct pci_resources_assigned2 *)(pkt + 1);
			res_assigned2->message_type.type =
				PCI_RESOURCES_ASSIGNED2;
			res_assigned2->wslot.slot = hpdev->desc.win_slot.slot;
		}
		put_pcichild(hpdev);

		ret = vmbus_sendpacket(hdev->channel, pkt + 1,
				size_res, (unsigned long)pkt,
				VM_PKT_DATA_INBAND,
				VMBUS_DATA_PACKET_FLAG_COMPLETION_REQUESTED);
		if (!ret)
			ret = wait_for_response(hdev, &comp_pkt.host_event);
		if (ret)
			break;

		if (comp_pkt.completion_status < 0) {
			ret = -EPROTO;
			dev_err(&hdev->device,
				"resource allocated returned 0x%x",
				comp_pkt.completion_status);
			break;
		}

		hbus->wslot_res_allocated = wslot;
	}

	kfree(pkt);
	return ret;
}
/* …（同文件无关代码省略）… */
static int hv_send_resources_released(struct hv_device *hdev)
{
	struct hv_pcibus_device *hbus = hv_get_drvdata(hdev);
	struct pci_child_message pkt;
	struct hv_pci_dev *hpdev;
	int wslot;
	int ret;

	for (wslot = hbus->wslot_res_allocated; wslot >= 0; wslot--) {
		hpdev = get_pcichild_wslot(hbus, wslot);
		if (!hpdev)
			continue;

		memset(&pkt, 0, sizeof(pkt));
		pkt.message_type.type = PCI_RESOURCES_RELEASED;
		pkt.wslot.slot = hpdev->desc.win_slot.slot;

		put_pcichild(hpdev);

		ret = vmbus_sendpacket(hdev->channel, &pkt, sizeof(pkt), 0,
				       VM_PKT_DATA_INBAND, 0);
		if (ret)
			return ret;

		hbus->wslot_res_allocated = wslot - 1;
	}

	hbus->wslot_res_allocated = -1;

	return 0;
}
/* …（同文件无关代码省略）… */
static int hv_pci_bus_exit(struct hv_device *hdev, bool keep_devs)
{
	struct hv_pcibus_device *hbus = hv_get_drvdata(hdev);
	struct vmbus_channel *chan = hdev->channel;
	struct {
		struct pci_packet teardown_packet;
		u8 buffer[sizeof(struct pci_message)];
	} pkt;
	struct pci_message *msg;
	struct hv_pci_compl comp_pkt;
	struct hv_pci_dev *hpdev, *tmp;
	unsigned long flags;
	u64 trans_id;
	int ret;

	/*
	 * After the host sends the RESCIND_CHANNEL message, it doesn't
	 * access the per-channel ringbuffer any longer.
	 */
	if (chan->rescind)
		return 0;

	if (!keep_devs) {
		struct list_head removed;

		/* Move all present children to the list on stack */
		INIT_LIST_HEAD(&removed);
		spin_lock_irqsave(&hbus->device_list_lock, flags);
		list_for_each_entry_safe(hpdev, tmp, &hbus->children, list_entry)
			list_move_tail(&hpdev->list_entry, &removed);
		spin_unlock_irqrestore(&hbus->device_list_lock, flags);

		/* Remove all children in the list */
		list_for_each_entry_safe(hpdev, tmp, &removed, list_entry) {
			list_del(&hpdev->list_entry);
			if (hpdev->pci_slot)
				pci_destroy_slot(hpdev->pci_slot);
			/* For the two refs got in new_pcichild_device() */
			put_pcichild(hpdev);
			put_pcichild(hpdev);
		}
	}

	ret = hv_send_resources_released(hdev);
	if (ret) {
		dev_err(&hdev->device,
			"Couldn't send resources released packet(s)\n");
		return ret;
	}

	memset(&pkt.teardown_packet, 0, sizeof(pkt.teardown_packet));
	init_completion(&comp_pkt.host_event);
	pkt.teardown_packet.completion_func = hv_pci_generic_compl;
	pkt.teardown_packet.compl_ctxt = &comp_pkt;
	msg = (struct pci_message *)pkt.buffer;
	msg->type = PCI_BUS_D0EXIT;

	ret = vmbus_sendpacket_getid(chan, msg, sizeof(*msg),
				     (unsigned long)&pkt.teardown_packet,
				     &trans_id, VM_PKT_DATA_INBAND,
				     VMBUS_DATA_PACKET_FLAG_COMPLETION_REQUESTED);
	if (ret)
		return ret;

	if (wait_for_completion_timeout(&comp_pkt.host_event, 10 * HZ) == 0) {
		/*
		 * The completion packet on the stack becomes invalid after
		 * 'return'; remove the ID from the VMbus requestor if the
		 * identifier is still mapped to/associated with the packet.
		 *
		 * Cf. hv_pci_onchannelcallback().
		 */
		vmbus_request_addr_match(chan, trans_id,
					 (unsigned long)&pkt.teardown_packet);
		return -ETIMEDOUT;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static void hv_pci_remove(struct hv_device *hdev)
{
	struct hv_pcibus_device *hbus;

	hbus = hv_get_drvdata(hdev);
	if (hbus->state == hv_pcibus_installed) {
		tasklet_disable(&hdev->channel->callback_event);
		hbus->state = hv_pcibus_removing;
		tasklet_enable(&hdev->channel->callback_event);
		destroy_workqueue(hbus->wq);
		hbus->wq = NULL;
		/*
		 * At this point, no work is running or can be scheduled
		 * on hbus-wq. We can't race with hv_pci_devices_present()
		 * or hv_pci_eject_device(), it's safe to proceed.
		 */

		/* Remove the bus from PCI's point of view. */
		pci_lock_rescan_remove();
		pci_stop_root_bus(hbus->bridge->bus);
		hv_pci_remove_slots(hbus);
		pci_remove_root_bus(hbus->bridge->bus);
		pci_unlock_rescan_remove();
	}

	hv_pci_bus_exit(hdev, false);

	vmbus_close(hdev->channel);

	iounmap(hbus->cfg_addr);
	hv_free_config_window(hbus);
	hv_pci_free_bridge_windows(hbus);
	irq_domain_remove(hbus->irq_domain);
	irq_domain_free_fwnode(hbus->fwnode);

	kfree(hbus);
}
/* …（同文件无关代码省略）… */
static int hv_pci_suspend(struct hv_device *hdev)
{
	struct hv_pcibus_device *hbus = hv_get_drvdata(hdev);
	enum hv_pcibus_state old_state;
	int ret;

	/*
	 * hv_pci_suspend() must make sure there are no pending work items
	 * before calling vmbus_close(), since it runs in a process context
	 * as a callback in dpm_suspend().  When it starts to run, the channel
	 * callback hv_pci_onchannelcallback(), which runs in a tasklet
	 * context, can be still running concurrently and scheduling new work
	 * items onto hbus->wq in hv_pci_devices_present() and
	 * hv_pci_eject_device(), and the work item handlers can access the
	 * vmbus channel, which can be being closed by hv_pci_suspend(), e.g.
	 * the work item handler pci_devices_present_work() ->
	 * new_pcichild_device() writes to the vmbus channel.
	 *
	 * To eliminate the race, hv_pci_suspend() disables the channel
	 * callback tasklet, sets hbus->state to hv_pcibus_removing, and
	 * re-enables the tasklet. This way, when hv_pci_suspend() proceeds,
	 * it knows that no new work item can be scheduled, and then it flushes
	 * hbus->wq and safely closes the vmbus channel.
	 */
	tasklet_disable(&hdev->channel->callback_event);

	/* Change the hbus state to prevent new work items. */
	old_state = hbus->state;
	if (hbus->state == hv_pcibus_installed)
		hbus->state = hv_pcibus_removing;

	tasklet_enable(&hdev->channel->callback_event);

	if (old_state != hv_pcibus_installed)
		return -EINVAL;

	flush_workqueue(hbus->wq);

	ret = hv_pci_bus_exit(hdev, true);
	if (ret)
		return ret;

	vmbus_close(hdev->channel);

	return 0;
}
/* …（同文件无关代码省略）… */
static int hv_pci_restore_msi_msg(struct pci_dev *pdev, void *arg)
{
	struct irq_data *irq_data;
	struct msi_desc *entry;

	if (!pdev->msi_enabled && !pdev->msix_enabled)
		return 0;

	guard(msi_descs_lock)(&pdev->dev);
	msi_for_each_desc(entry, &pdev->dev, MSI_DESC_ASSOCIATED) {
		irq_data = irq_get_irq_data(entry->irq);
		if (WARN_ON_ONCE(!irq_data))
			return -EINVAL;
		hv_compose_msi_msg(irq_data, &entry->msg);
	}
	return 0;
}
/* …（同文件无关代码省略）… */
static void hv_pci_restore_msi_state(struct hv_pcibus_device *hbus)
{
	pci_walk_bus(hbus->bridge->bus, hv_pci_restore_msi_msg, NULL);
}
/* …（同文件无关代码省略）… */
static int hv_pci_resume(struct hv_device *hdev)
{
	struct hv_pcibus_device *hbus = hv_get_drvdata(hdev);
	enum pci_protocol_version_t version[1];
	int ret;

	hbus->state = hv_pcibus_init;

	hdev->channel->next_request_id_callback = vmbus_next_request_id;
	hdev->channel->request_addr_callback = vmbus_request_addr;
	hdev->channel->rqstor_size = HV_PCI_RQSTOR_SIZE;

	ret = vmbus_open(hdev->channel, pci_ring_size, pci_ring_size, NULL, 0,
			 hv_pci_onchannelcallback, hbus);
	if (ret)
		return ret;

	/* Only use the version that was in use before hibernation. */
	version[0] = hbus->protocol_version;
	ret = hv_pci_protocol_negotiation(hdev, version, 1);
	if (ret)
		goto out;

	ret = hv_pci_query_relations(hdev);
	if (ret)
		goto out;

	mutex_lock(&hbus->state_lock);

	ret = hv_pci_enter_d0(hdev);
	if (ret)
		goto release_state_lock;

	ret = hv_send_resources_allocated(hdev);
	if (ret)
		goto release_state_lock;

	prepopulate_bars(hbus);

	hv_pci_restore_msi_state(hbus);

	hbus->state = hv_pcibus_installed;
	mutex_unlock(&hbus->state_lock);
	return 0;

release_state_lock:
	mutex_unlock(&hbus->state_lock);
out:
	vmbus_close(hdev->channel);
	return ret;
}
/* …（同文件无关代码省略）… */
	.remove		= hv_pci_remove,
	.suspend	= hv_pci_suspend,
	.resume		= hv_pci_resume,
};

static void __exit exit_hv_pci_drv(void)
{
	vmbus_driver_unregister(&hv_pci_drv);

	hvpci_block_ops.read_block = NULL;
	hvpci_block_ops.write_block = NULL;
	hvpci_block_ops.reg_blk_invalidate = NULL;
}
