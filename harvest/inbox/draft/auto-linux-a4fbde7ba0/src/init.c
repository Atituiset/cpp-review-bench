// AUTO-DRAFT from torvalds/linux PR #a7963b12ca3e764e6ad3b15d1373430eb02492ee
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#define IVRS_HEADER_LENGTH 48
/* …（同文件无关代码省略）… */
#define ACPI_IVMD_TYPE_ALL              0x20
#define ACPI_IVMD_TYPE                  0x21
#define ACPI_IVMD_TYPE_RANGE            0x22
/* …（同文件无关代码省略）… */
#define IVHD_DEV_ALL                    0x01
#define IVHD_DEV_SELECT                 0x02
#define IVHD_DEV_SELECT_RANGE_START     0x03
#define IVHD_DEV_RANGE_END              0x04
#define IVHD_DEV_ALIAS                  0x42
#define IVHD_DEV_ALIAS_RANGE            0x43
#define IVHD_DEV_EXT_SELECT             0x46
#define IVHD_DEV_EXT_SELECT_RANGE       0x47
#define IVHD_DEV_SPECIAL		0x48
#define IVHD_DEV_ACPI_HID		0xf0
/* …（同文件无关代码省略）… */
#define UID_NOT_PRESENT                 0
#define UID_IS_INTEGER                  1
#define UID_IS_CHARACTER                2
/* …（同文件无关代码省略）… */
#define IVHD_SPECIAL_IOAPIC		1
#define IVHD_SPECIAL_HPET		2
/* …（同文件无关代码省略）… */
#define IVHD_FLAG_HT_TUN_EN_MASK        0x01
#define IVHD_FLAG_PASSPW_EN_MASK        0x02
#define IVHD_FLAG_RESPASSPW_EN_MASK     0x04
#define IVHD_FLAG_ISOC_EN_MASK          0x08
/* …（同文件无关代码省略）… */
#define IVMD_FLAG_EXCL_RANGE            0x08
#define IVMD_FLAG_IW                    0x04
#define IVMD_FLAG_IR                    0x02
#define IVMD_FLAG_UNITY_MAP             0x01
/* …（同文件无关代码省略）… */
#define ACPI_DEVFLAG_INITPASS           0x01
#define ACPI_DEVFLAG_EXTINT             0x02
#define ACPI_DEVFLAG_NMI                0x04
#define ACPI_DEVFLAG_SYSMGT1            0x10
#define ACPI_DEVFLAG_SYSMGT2            0x20
#define ACPI_DEVFLAG_LINT0              0x40
#define ACPI_DEVFLAG_LINT1              0x80
/* …（同文件无关代码省略）… */
struct ivhd_header {
	u8 type;
	u8 flags;
	u16 length;
	u16 devid;
	u16 cap_ptr;
	u64 mmio_phys;
	u16 pci_seg;
	u16 info;
	u32 efr_attr;

	/* Following only valid on IVHD type 11h and 40h */
	u64 efr_reg; /* Exact copy of MMIO_EXT_FEATURES */
	u64 efr_reg2;
} __attribute__((packed));
/* …（同文件无关代码省略）… */
struct ivhd_entry {
	u8 type;
	u16 devid;
	u8 flags;
	struct_group(ext_hid,
		u32 ext;
		u32 hidh;
	);
	u64 cid;
	u8 uidf;
	u8 uidl;
	u8 uid;
} __attribute__((packed));
/* …（同文件无关代码省略）… */
struct ivmd_header {
	u8 type;
	u8 flags;
	u16 length;
	u16 devid;
	u16 aux;
	u16 pci_seg;
	u8  resv[6];
	u64 range_start;
	u64 range_length;
} __attribute__((packed));
/* …（同文件无关代码省略）… */
static void iommu_set_device_table(struct amd_iommu *iommu)
{
	u64 entry;
	u32 dev_table_size = iommu->pci_seg->dev_table_size;
	void *dev_table = (void *)get_dev_table(iommu);

	BUG_ON(iommu->mmio_base == NULL);

	/*
	 * A kdump kernel that reuses the previous kernel's device table must
	 * leave the base register alone. It already points at that table, and
	 * with SNP enabled the register is locked. If reuse failed, program
	 * the freshly allocated table like a normal boot.
	 */
	if (iommu->pci_seg->old_dev_tbl_cpy)
		return;

	entry = iommu_virt_to_phys(dev_table);
	entry |= (dev_table_size >> 12) - 1;
	memcpy_toio(iommu->mmio_base + MMIO_DEV_TABLE_OFFSET,
			&entry, sizeof(entry));
}
/* …（同文件无关代码省略）… */
static void iommu_feature_set(struct amd_iommu *iommu, u64 val, u64 mask, u8 shift)
{
	u64 ctrl;

	ctrl = readq(iommu->mmio_base +  MMIO_CONTROL_OFFSET);
	mask <<= shift;
	ctrl &= ~mask;
	ctrl |= (val << shift) & mask;
	writeq(ctrl, iommu->mmio_base +  MMIO_CONTROL_OFFSET);
}
/* …（同文件无关代码省略）… */
void iommu_feature_enable(struct amd_iommu *iommu, u8 bit)
{
	iommu_feature_set(iommu, 1ULL, 1ULL, bit);
}
/* …（同文件无关代码省略）… */
static void iommu_feature_disable(struct amd_iommu *iommu, u8 bit)
{
	iommu_feature_set(iommu, 0ULL, 1ULL, bit);
}
/* …（同文件无关代码省略）… */
static void iommu_enable(struct amd_iommu *iommu)
{
	iommu_feature_enable(iommu, CONTROL_IOMMU_EN);
}

static void iommu_disable(struct amd_iommu *iommu)
{
	if (!iommu->mmio_base)
		return;

/* …（同文件无关代码省略）… */

	/* Clear IRTE cache disabling bit */
	iommu_feature_disable(iommu, CONTROL_IRTCACHEDIS);
}

/*
/* …（同文件无关代码省略）… */
static inline u32 get_ivhd_header_size(struct ivhd_header *h)
{
	u32 size = 0;

	switch (h->type) {
	case 0x10:
		size = 24;
		break;
	case 0x11:
	case 0x40:
		size = 40;
		break;
	}
	return size;
}
/* …（同文件无关代码省略）… */
static inline int ivhd_entry_length(u8 *ivhd)
{
	u32 type = ((struct ivhd_entry *)ivhd)->type;

	if (type < 0x80) {
		return 0x04 << (*ivhd >> 6);
	} else if (type == IVHD_DEV_ACPI_HID) {
		/* For ACPI_HID, offset 21 is uid len */
		return *((u8 *)ivhd + 21) + 22;
	}
	return 0;
}
/* …（同文件无关代码省略）… */
static int __init find_last_devid_from_ivhd(struct ivhd_header *h)
{
	u8 *p = (void *)h, *end = (void *)h;
	struct ivhd_entry *dev;
	int last_devid = -EINVAL;

	u32 ivhd_size = get_ivhd_header_size(h);

	if (!ivhd_size) {
		pr_err("Unsupported IVHD type %#x\n", h->type);
		return -EINVAL;
	}

	p += ivhd_size;
	end += h->length;

	while (p < end) {
		dev = (struct ivhd_entry *)p;
		switch (dev->type) {
		case IVHD_DEV_ALL:
			/* Use maximum BDF value for DEV_ALL */
			return 0xffff;
		case IVHD_DEV_SELECT:
		case IVHD_DEV_RANGE_END:
		case IVHD_DEV_ALIAS:
		case IVHD_DEV_EXT_SELECT:
			/* all the above subfield types refer to device ids */
			if (dev->devid > last_devid)
				last_devid = dev->devid;
			break;
		default:
			break;
		}
		p += ivhd_entry_length(p);
	}

	WARN_ON(p != end);

	return last_devid;
}
/* …（同文件无关代码省略）… */
static int __init find_last_devid_acpi(struct acpi_table_header *table, u16 pci_seg)
{
	u8 *p = (u8 *)table, *end = (u8 *)table;
	struct ivhd_header *h;
	int last_devid, last_bdf = 0;

	p += IVRS_HEADER_LENGTH;

	end += table->length;
	while (p < end) {
		h = (struct ivhd_header *)p;
		if (h->pci_seg == pci_seg &&
		    h->type == amd_iommu_target_ivhd_type) {
			last_devid = find_last_devid_from_ivhd(h);

			if (last_devid < 0)
				return -EINVAL;
			if (last_devid > last_bdf)
				last_bdf = last_devid;
		}
		p += h->length;
	}
	WARN_ON(p != end);

	return last_bdf;
}
/* …（同文件无关代码省略）… */
static inline int __init alloc_dev_table(struct amd_iommu_pci_seg *pci_seg)
{
	gfp_t gfp = GFP_KERNEL;

	/*
	 * Keep normal kernel device tables below 4G because a later kdump
	 * kernel only trusts and reuses old tables below that limit. A kdump
	 * kernel's new table is either discarded when reuse succeeds or
	 * programmed after the IOMMU is disabled, so the table does not
	 * need DMA32 memory (which the crashkernel may not have).
	 */
	if (!is_kdump_kernel())
		gfp |= GFP_DMA32;

	pci_seg->dev_table = iommu_alloc_pages_sz(gfp, pci_seg->dev_table_size);
	if (!pci_seg->dev_table)
		return -ENOMEM;

	return 0;
}
/* …（同文件无关代码省略）… */
static inline void free_dev_table(struct amd_iommu_pci_seg *pci_seg)
{
	if (is_kdump_kernel())
		memunmap((void *)pci_seg->dev_table);
	else
		iommu_free_pages(pci_seg->dev_table);
	pci_seg->dev_table = NULL;
}
/* …（同文件无关代码省略）… */
static inline int __init alloc_rlookup_table(struct amd_iommu_pci_seg *pci_seg)
{
	pci_seg->rlookup_table = kvzalloc_objs(*pci_seg->rlookup_table,
					       pci_seg->last_bdf + 1);
	if (pci_seg->rlookup_table == NULL)
		return -ENOMEM;

	return 0;
}
/* …（同文件无关代码省略）… */
static int __init alloc_alias_table(struct amd_iommu_pci_seg *pci_seg)
{
	int i;

	pci_seg->alias_table = kvmalloc_objs(*pci_seg->alias_table,
					     pci_seg->last_bdf + 1);
	if (!pci_seg->alias_table)
		return -ENOMEM;

	/*
	 * let all alias entries point to itself
	 */
	for (i = 0; i <= pci_seg->last_bdf; ++i)
		pci_seg->alias_table[i] = i;

	return 0;
}
/* …（同文件无关代码省略）… */
static void __init free_alias_table(struct amd_iommu_pci_seg *pci_seg)
{
	kvfree(pci_seg->alias_table);
	pci_seg->alias_table = NULL;
}
/* …（同文件无关代码省略）… */
static void amd_iommu_reset_cmd_buffer(struct amd_iommu *iommu)
{
	iommu_feature_disable(iommu, CONTROL_CMDBUF_EN);

	writel(0x00, iommu->mmio_base + MMIO_CMD_HEAD_OFFSET);
	writel(0x00, iommu->mmio_base + MMIO_CMD_TAIL_OFFSET);
	iommu->cmd_buf_head = 0;
	iommu->cmd_buf_tail = 0;

	iommu_feature_enable(iommu, CONTROL_CMDBUF_EN);
}
/* …（同文件无关代码省略）… */
static void iommu_enable_command_buffer(struct amd_iommu *iommu)
{
	u64 entry;

	BUG_ON(iommu->cmd_buf == NULL);

	if (!is_kdump_kernel()) {
		/*
		 * Command buffer is re-used for kdump kernel and setting
		 * of MMIO register is not required.
		 */
		entry = iommu_virt_to_phys(iommu->cmd_buf);
		entry |= MMIO_CMD_SIZE_512;
		memcpy_toio(iommu->mmio_base + MMIO_CMD_BUF_OFFSET,
			    &entry, sizeof(entry));
	}

	amd_iommu_reset_cmd_buffer(iommu);
}
/* …（同文件无关代码省略）… */
static void iommu_enable_event_buffer(void)
{
	struct amd_iommu *iommu;
	u64 entry;

	for_each_iommu(iommu) {
		BUG_ON(iommu->evt_buf == NULL);

		if (!is_kdump_kernel()) {
			/*
			 * Event buffer is re-used for kdump kernel and setting
			 * of MMIO register is not required.
			 */
			entry = iommu_virt_to_phys(iommu->evt_buf);
			entry |= (amd_iommu_evtlog_size == EVTLOG_SIZE_DEF) ?
				EVTLOG_LEN_MASK_DEF : EVTLOG_LEN_MASK_MAX;

			memcpy_toio(iommu->mmio_base + MMIO_EVT_BUF_OFFSET,
				    &entry, sizeof(entry));
		}

		/* set head and tail to zero manually */
		writel(0x00, iommu->mmio_base + MMIO_EVT_HEAD_OFFSET);
		writel(0x00, iommu->mmio_base + MMIO_EVT_TAIL_OFFSET);

		iommu_feature_enable(iommu, CONTROL_EVT_LOG_EN);
	}
}
/* …（同文件无关代码省略）… */
static bool iommu_ga_log_enable(struct amd_iommu *iommu)
{
	u32 status, i;
	u64 entry;

	if (!iommu->ga_log)
		return false;

	entry = iommu_virt_to_phys(iommu->ga_log) | GA_LOG_SIZE_512;
	memcpy_toio(iommu->mmio_base + MMIO_GA_LOG_BASE_OFFSET,
		    &entry, sizeof(entry));
	entry = (iommu_virt_to_phys(iommu->ga_log_tail) &
		 (BIT_ULL(52)-1)) & ~7ULL;
	memcpy_toio(iommu->mmio_base + MMIO_GA_LOG_TAIL_OFFSET,
		    &entry, sizeof(entry));
	writel(0x00, iommu->mmio_base + MMIO_GA_HEAD_OFFSET);
	writel(0x00, iommu->mmio_base + MMIO_GA_TAIL_OFFSET);


	iommu_feature_enable(iommu, CONTROL_GAINT_EN);
	iommu_feature_enable(iommu, CONTROL_GALOG_EN);

	for (i = 0; i < MMIO_STATUS_TIMEOUT; ++i) {
		status = readl(iommu->mmio_base + MMIO_STATUS_OFFSET);
		if (status & (MMIO_STATUS_GALOG_RUN_MASK))
			break;
		udelay(10);
	}

	if (WARN_ON(i >= MMIO_STATUS_TIMEOUT))
		return false;

	return true;
}
/* …（同文件无关代码省略）… */
static void iommu_enable_xt(struct amd_iommu *iommu)
{
#ifdef CONFIG_IRQ_REMAP
	/*
	 * XT mode (32-bit APIC destination ID) requires
	 * GA mode (128-bit IRTE support) as a prerequisite.
	 */
	if (AMD_IOMMU_GUEST_IR_GA(amd_iommu_guest_ir) &&
	    amd_iommu_xt_mode == IRQ_REMAP_X2APIC_MODE)
		iommu_feature_enable(iommu, CONTROL_XT_EN);
#endif /* CONFIG_IRQ_REMAP */
}
/* …（同文件无关代码省略）… */
static void iommu_enable_gt(struct amd_iommu *iommu)
{
	if (!check_feature(FEATURE_GT))
		return;

	iommu_feature_enable(iommu, CONTROL_GT_EN);

	/*
	 * This feature needs to be enabled prior to a call
	 * to iommu_snp_enable(). Since this function is called
	 * in early_enable_iommu(), it is safe to enable here.
	 */
	if (check_feature2(FEATURE_GCR3TRPMODE))
		iommu_feature_enable(iommu, CONTROL_GCR3TRPMODE);
}
/* …（同文件无关代码省略）… */
static void set_dte_bit(struct dev_table_entry *dte, u8 bit)
{
	int i = (bit >> 6) & 0x03;
	int _bit = bit & 0x3f;

	dte->data[i] |= (1UL << _bit);
}
/* …（同文件无关代码省略）… */
static bool search_ivhd_dte_flags(u16 segid, u16 first, u16 last)
{
	struct ivhd_dte_flags *e;

	for_each_ivhd_dte_flags(e) {
		if ((e->segid == segid) &&
		    (e->devid_first == first) &&
		    (e->devid_last == last))
			return true;
	}
	return false;
}
/* …（同文件无关代码省略）… */
static void __init
set_dev_entry_from_acpi_range(struct amd_iommu *iommu, u16 first, u16 last,
			      u32 flags, u32 ext_flags)
{
	int i;
	struct dev_table_entry dte = {};

	/* Parse IVHD DTE setting flags and store information */
	if (flags) {
		struct ivhd_dte_flags *d;

		if (search_ivhd_dte_flags(iommu->pci_seg->id, first, last))
			return;

		d = kzalloc_obj(struct ivhd_dte_flags);
		if (!d)
			return;

		pr_debug("%s: devid range %#x:%#x\n", __func__, first, last);

		if (flags & ACPI_DEVFLAG_INITPASS)
			set_dte_bit(&dte, DEV_ENTRY_INIT_PASS);
		if (flags & ACPI_DEVFLAG_EXTINT)
			set_dte_bit(&dte, DEV_ENTRY_EINT_PASS);
		if (flags & ACPI_DEVFLAG_NMI)
			set_dte_bit(&dte, DEV_ENTRY_NMI_PASS);
		if (flags & ACPI_DEVFLAG_SYSMGT1)
			set_dte_bit(&dte, DEV_ENTRY_SYSMGT1);
		if (flags & ACPI_DEVFLAG_SYSMGT2)
			set_dte_bit(&dte, DEV_ENTRY_SYSMGT2);
		if (flags & ACPI_DEVFLAG_LINT0)
			set_dte_bit(&dte, DEV_ENTRY_LINT0_PASS);
		if (flags & ACPI_DEVFLAG_LINT1)
			set_dte_bit(&dte, DEV_ENTRY_LINT1_PASS);

		/* Apply erratum 63, which needs info in initial_dte */
		if (FIELD_GET(DTE_DATA1_SYSMGT_MASK, dte.data[1]) == 0x1)
			dte.data[0] |= DTE_FLAG_IW;

		memcpy(&d->dte, &dte, sizeof(dte));
		d->segid = iommu->pci_seg->id;
		d->devid_first = first;
		d->devid_last = last;
		list_add_tail(&d->list, &amd_ivhd_dev_flags_list);
	}

	for (i = first; i <= last; i++)  {
		if (flags) {
			struct dev_table_entry *dev_table = get_dev_table(iommu);

			memcpy(&dev_table[i], &dte, sizeof(dte));
		}
		amd_iommu_set_rlookup_table(iommu, i);
	}
}
/* …（同文件无关代码省略）… */
static void __init set_dev_entry_from_acpi(struct amd_iommu *iommu,
					   u16 devid, u32 flags, u32 ext_flags)
{
	set_dev_entry_from_acpi_range(iommu, devid, devid, flags, ext_flags);
}
/* …（同文件无关代码省略）… */
int __init add_special_device(u8 type, u8 id, u32 *devid, bool cmd_line)
{
	struct devid_map *entry;
	struct list_head *list;

	if (type == IVHD_SPECIAL_IOAPIC)
		list = &ioapic_map;
	else if (type == IVHD_SPECIAL_HPET)
		list = &hpet_map;
	else
		return -EINVAL;

	list_for_each_entry(entry, list, list) {
		if (!(entry->id == id && entry->cmd_line))
			continue;

		pr_info("Command-line override present for %s id %d - ignoring\n",
			type == IVHD_SPECIAL_IOAPIC ? "IOAPIC" : "HPET", id);

		*devid = entry->devid;

		return 0;
	}

	entry = kzalloc_obj(*entry);
	if (!entry)
		return -ENOMEM;

	entry->id	= id;
	entry->devid	= *devid;
	entry->cmd_line	= cmd_line;

	list_add_tail(&entry->list, list);

	return 0;
}
/* …（同文件无关代码省略）… */
static int __init add_acpi_hid_device(u8 *hid, u8 *uid, u32 *devid,
				      bool cmd_line)
{
	struct acpihid_map_entry *entry;
	struct list_head *list = &acpihid_map;

	list_for_each_entry(entry, list, list) {
		if (strcmp(entry->hid, hid) ||
		    (*uid && *entry->uid && strcmp(entry->uid, uid)) ||
		    !entry->cmd_line)
			continue;

		pr_info("Command-line override for hid:%s uid:%s\n",
			hid, uid);
		*devid = entry->devid;
		return 0;
	}

	entry = kzalloc_obj(*entry);
	if (!entry)
		return -ENOMEM;

	memcpy(entry->uid, uid, strlen(uid));
	memcpy(entry->hid, hid, strlen(hid));
	entry->devid = *devid;
	entry->cmd_line	= cmd_line;
	entry->root_devid = (entry->devid & (~0x7));

	pr_info("%s, add hid:%s, uid:%s, rdevid:%#x\n",
		entry->cmd_line ? "cmd" : "ivrs",
		entry->hid, entry->uid, entry->root_devid);

	list_add_tail(&entry->list, list);
	return 0;
}
/* …（同文件无关代码省略）… */
static int __init add_early_maps(void)
{
	int i, ret;

	for (i = 0; i < early_ioapic_map_size; ++i) {
		ret = add_special_device(IVHD_SPECIAL_IOAPIC,
					 early_ioapic_map[i].id,
					 &early_ioapic_map[i].devid,
					 early_ioapic_map[i].cmd_line);
		if (ret)
			return ret;
	}

	for (i = 0; i < early_hpet_map_size; ++i) {
		ret = add_special_device(IVHD_SPECIAL_HPET,
					 early_hpet_map[i].id,
					 &early_hpet_map[i].devid,
					 early_hpet_map[i].cmd_line);
		if (ret)
			return ret;
	}

	for (i = 0; i < early_acpihid_map_size; ++i) {
		ret = add_acpi_hid_device(early_acpihid_map[i].hid,
					  early_acpihid_map[i].uid,
					  &early_acpihid_map[i].devid,
					  early_acpihid_map[i].cmd_line);
		if (ret)
			return ret;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static int __init init_iommu_from_acpi(struct amd_iommu *iommu,
					struct ivhd_header *h)
{
	u8 *p = (u8 *)h;
	u8 *end = p, flags = 0;
	u16 devid = 0, devid_start = 0, devid_to = 0, seg_id;
	u32 dev_i, ext_flags = 0;
	bool alias = false;
	struct ivhd_entry *e;
	struct amd_iommu_pci_seg *pci_seg = iommu->pci_seg;
	u32 ivhd_size;
	int ret;


	ret = add_early_maps();
	if (ret)
		return ret;

	amd_iommu_apply_ivrs_quirks();

	/*
	 * First save the recommended feature enable bits from ACPI
	 */
	iommu->acpi_flags = h->flags;

	/*
	 * Done. Now parse the device entries
	 */
	ivhd_size = get_ivhd_header_size(h);
	if (!ivhd_size) {
		pr_err("Unsupported IVHD type %#x\n", h->type);
		return -EINVAL;
	}

	p += ivhd_size;

	end += h->length;


	while (p < end) {
		e = (struct ivhd_entry *)p;
		seg_id = pci_seg->id;

		switch (e->type) {
		case IVHD_DEV_ALL:

			DUMP_printk("  DEV_ALL\t\t\tsetting: %#02x\n", e->flags);
			set_dev_entry_from_acpi_range(iommu, 0, pci_seg->last_bdf, e->flags, 0);
			break;
		case IVHD_DEV_SELECT:

			DUMP_printk("  DEV_SELECT\t\t\tdevid: %04x:%02x:%02x.%x flags: %#02x\n",
				    seg_id, PCI_BUS_NUM(e->devid),
				    PCI_SLOT(e->devid),
				    PCI_FUNC(e->devid),
				    e->flags);

			devid = e->devid;
			set_dev_entry_from_acpi(iommu, devid, e->flags, 0);
			break;
		case IVHD_DEV_SELECT_RANGE_START:

			DUMP_printk("  DEV_SELECT_RANGE_START\tdevid: %04x:%02x:%02x.%x flags: %#02x\n",
				    seg_id, PCI_BUS_NUM(e->devid),
				    PCI_SLOT(e->devid),
				    PCI_FUNC(e->devid),
				    e->flags);

			devid_start = e->devid;
			flags = e->flags;
			ext_flags = 0;
			alias = false;
			break;
		case IVHD_DEV_ALIAS:

			DUMP_printk("  DEV_ALIAS\t\t\tdevid: %04x:%02x:%02x.%x flags: %#02x devid_to: %02x:%02x.%x\n",
				    seg_id, PCI_BUS_NUM(e->devid),
				    PCI_SLOT(e->devid),
				    PCI_FUNC(e->devid),
				    e->flags,
				    PCI_BUS_NUM(e->ext >> 8),
				    PCI_SLOT(e->ext >> 8),
				    PCI_FUNC(e->ext >> 8));

			devid = e->devid;
			devid_to = e->ext >> 8;
			set_dev_entry_from_acpi(iommu, devid   , e->flags, 0);
			set_dev_entry_from_acpi(iommu, devid_to, e->flags, 0);
			pci_seg->alias_table[devid] = devid_to;
			break;
		case IVHD_DEV_ALIAS_RANGE:

			DUMP_printk("  DEV_ALIAS_RANGE\t\tdevid: %04x:%02x:%02x.%x flags: %#02x devid_to: %04x:%02x:%02x.%x\n",
				    seg_id, PCI_BUS_NUM(e->devid),
				    PCI_SLOT(e->devid),
				    PCI_FUNC(e->devid),
				    e->flags,
				    seg_id, PCI_BUS_NUM(e->ext >> 8),
				    PCI_SLOT(e->ext >> 8),
				    PCI_FUNC(e->ext >> 8));

			devid_start = e->devid;
			flags = e->flags;
			devid_to = e->ext >> 8;
			ext_flags = 0;
			alias = true;
			break;
		case IVHD_DEV_EXT_SELECT:

			DUMP_printk("  DEV_EXT_SELECT\t\tdevid: %04x:%02x:%02x.%x flags: %#02x ext: %08x\n",
				    seg_id, PCI_BUS_NUM(e->devid),
				    PCI_SLOT(e->devid),
				    PCI_FUNC(e->devid),
				    e->flags, e->ext);

			devid = e->devid;
			set_dev_entry_from_acpi(iommu, devid, e->flags,
						e->ext);
			break;
		case IVHD_DEV_EXT_SELECT_RANGE:

			DUMP_printk("  DEV_EXT_SELECT_RANGE\tdevid: %04x:%02x:%02x.%x flags: %#02x ext: %08x\n",
				    seg_id, PCI_BUS_NUM(e->devid),
				    PCI_SLOT(e->devid),
				    PCI_FUNC(e->devid),
				    e->flags, e->ext);

			devid_start = e->devid;
			flags = e->flags;
			ext_flags = e->ext;
			alias = false;
			break;
		case IVHD_DEV_RANGE_END:

			DUMP_printk("  DEV_RANGE_END\t\tdevid: %04x:%02x:%02x.%x\n",
				    seg_id, PCI_BUS_NUM(e->devid),
				    PCI_SLOT(e->devid),
				    PCI_FUNC(e->devid));

			devid = e->devid;
			if (alias) {
				for (dev_i = devid_start; dev_i <= devid; ++dev_i)
					pci_seg->alias_table[dev_i] = devid_to;
				set_dev_entry_from_acpi(iommu, devid_to, flags, ext_flags);
			}
			set_dev_entry_from_acpi_range(iommu, devid_start, devid, flags, ext_flags);
			break;
		case IVHD_DEV_SPECIAL: {
			u8 handle, type;
			const char *var;
			u32 devid;
			int ret;

			handle = e->ext & 0xff;
			devid = PCI_SEG_DEVID_TO_SBDF(seg_id, (e->ext >> 8));
			type   = (e->ext >> 24) & 0xff;

			if (type == IVHD_SPECIAL_IOAPIC)
				var = "IOAPIC";
			else if (type == IVHD_SPECIAL_HPET)
				var = "HPET";
			else
				var = "UNKNOWN";

			DUMP_printk("  DEV_SPECIAL(%s[%d])\t\tdevid: %04x:%02x:%02x.%x, flags: %#02x\n",
				    var, (int)handle,
				    seg_id, PCI_BUS_NUM(devid),
				    PCI_SLOT(devid),
				    PCI_FUNC(devid),
				    e->flags);

			ret = add_special_device(type, handle, &devid, false);
			if (ret)
				return ret;

			/*
			 * add_special_device might update the devid in case a
			 * command-line override is present. So call
			 * set_dev_entry_from_acpi after add_special_device.
			 */
			set_dev_entry_from_acpi(iommu, devid, e->flags, 0);

			break;
		}
		case IVHD_DEV_ACPI_HID: {
			u32 devid;
			u8 hid[ACPIHID_HID_LEN];
			u8 uid[ACPIHID_UID_LEN];
			int ret;

			if (h->type != 0x40) {
				pr_err(FW_BUG "Invalid IVHD device type %#x\n",
				       e->type);
				break;
			}

			BUILD_BUG_ON(sizeof(e->ext_hid) != ACPIHID_HID_LEN - 1);
			memcpy(hid, &e->ext_hid, ACPIHID_HID_LEN - 1);
			hid[ACPIHID_HID_LEN - 1] = '\0';

			if (!(*hid)) {
				pr_err(FW_BUG "Invalid HID.\n");
				break;
			}

			uid[0] = '\0';
			switch (e->uidf) {
			case UID_NOT_PRESENT:

				if (e->uidl != 0)
					pr_warn(FW_BUG "Invalid UID length.\n");

				break;
			case UID_IS_INTEGER:

				sprintf(uid, "%d", e->uid);

				break;
			case UID_IS_CHARACTER:

				memcpy(uid, &e->uid, e->uidl);
				uid[e->uidl] = '\0';

				break;
			default:
				break;
			}

			devid = PCI_SEG_DEVID_TO_SBDF(seg_id, e->devid);
			DUMP_printk("  DEV_ACPI_HID(%s[%s])\t\tdevid: %04x:%02x:%02x.%x, flags: %#02x\n",
				    hid, uid, seg_id,
				    PCI_BUS_NUM(devid),
				    PCI_SLOT(devid),
				    PCI_FUNC(devid),
				    e->flags);

			flags = e->flags;

			ret = add_acpi_hid_device(hid, uid, &devid, false);
			if (ret)
				return ret;

			/*
			 * add_special_device might update the devid in case a
			 * command-line override is present. So call
			 * set_dev_entry_from_acpi after add_special_device.
			 */
			set_dev_entry_from_acpi(iommu, devid, e->flags, 0);

			break;
		}
		default:
			break;
		}

		p += ivhd_entry_length(p);
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static struct amd_iommu_pci_seg *__init alloc_pci_segment(u16 id,
					  struct acpi_table_header *ivrs_base)
{
	struct amd_iommu_pci_seg *pci_seg;
	int last_bdf;

	/*
	 * First parse ACPI tables to find the largest Bus/Dev/Func we need to
	 * handle in this PCI segment. Upon this information the shared data
	 * structures for the PCI segments in the system will be allocated.
	 */
	last_bdf = find_last_devid_acpi(ivrs_base, id);
	if (last_bdf < 0)
		return NULL;

	pci_seg = kzalloc_obj(struct amd_iommu_pci_seg);
	if (pci_seg == NULL)
		return NULL;

	pci_seg->last_bdf = last_bdf;
	DUMP_printk("PCI segment : 0x%0x, last bdf : 0x%04x\n", id, last_bdf);
	pci_seg->dev_table_size =
		max(roundup_pow_of_two((last_bdf + 1) * DEV_TABLE_ENTRY_SIZE),
		    SZ_4K);

	pci_seg->id = id;
	init_llist_head(&pci_seg->dev_data_list);
	INIT_LIST_HEAD(&pci_seg->unity_map);
	list_add_tail(&pci_seg->list, &amd_iommu_pci_seg_list);

	if (alloc_dev_table(pci_seg))
		goto err_free_pci_seg;
	if (alloc_alias_table(pci_seg))
		goto err_free_dev_table;
	if (alloc_rlookup_table(pci_seg))
		goto err_free_alias_table;

	return pci_seg;

err_free_alias_table:
	free_alias_table(pci_seg);
err_free_dev_table:
	free_dev_table(pci_seg);
err_free_pci_seg:
	list_del(&pci_seg->list);
	kfree(pci_seg);
	return NULL;
}
/* …（同文件无关代码省略）… */
static struct amd_iommu_pci_seg *__init get_pci_segment(u16 id,
					struct acpi_table_header *ivrs_base)
{
	struct amd_iommu_pci_seg *pci_seg;

	for_each_pci_segment(pci_seg) {
		if (pci_seg->id == id)
			return pci_seg;
	}

	return alloc_pci_segment(id, ivrs_base);
}
/* …（同文件无关代码省略）… */
				 struct acpi_table_header *ivrs_base)
{
	struct amd_iommu_pci_seg *pci_seg;

	pci_seg = get_pci_segment(h->pci_seg, ivrs_base);
	if (pci_seg == NULL)
/* …（同文件无关代码省略）… */
	if (!iommu->mmio_base)
		return -ENOMEM;

	if (amd_iommu_perfopt_clear(iommu))
		pr_err("IOMMU%d: failed to clear PerfOpt\n", iommu->index);

	return init_iommu_from_acpi(iommu, h);
}
/* …（同文件无关代码省略）… */
		if (check_feature2(FEATURE_SEVSNPIO_SUP))
			pr_cont(" SEV-TIO");

		pr_cont("\n");
	}

/* …（同文件无关代码省略）… */
static int iommu_setup_msi(struct amd_iommu *iommu)
{
	int r;

	r = pci_enable_msi(iommu->dev);
	if (r)
		return r;

	r = request_threaded_irq(iommu->dev->irq, NULL, amd_iommu_int_thread,
				 IRQF_ONESHOT, "AMD-Vi", iommu);
	if (r) {
		pci_disable_msi(iommu->dev);
		return r;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static struct irq_domain *iommu_get_irqdomain(void)
{
	struct fwnode_handle *fn;

	/* No need for locking here (yet) as the init is single-threaded */
	if (iommu_irqdomain)
		return iommu_irqdomain;

	fn = irq_domain_alloc_named_fwnode("AMD-Vi-MSI");
	if (!fn)
		return NULL;

	iommu_irqdomain = irq_domain_create_hierarchy(x86_vector_domain, 0, 0,
						      fn, &intcapxt_domain_ops,
						      NULL);
	if (!iommu_irqdomain)
		irq_domain_free_fwnode(fn);

	return iommu_irqdomain;
}
/* …（同文件无关代码省略）… */
static int __iommu_setup_intcapxt(struct amd_iommu *iommu, const char *devname,
				  int hwirq, irq_handler_t thread_fn)
{
	struct irq_domain *domain;
	struct irq_alloc_info info;
	int irq, ret;
	int node = dev_to_node(&iommu->dev->dev);

	domain = iommu_get_irqdomain();
	if (!domain)
		return -ENXIO;

	init_irq_alloc_info(&info, NULL);
	info.type = X86_IRQ_ALLOC_TYPE_AMDVI;
	info.data = iommu;
	info.hwirq = hwirq;

	irq = irq_domain_alloc_irqs(domain, 1, node, &info);
	if (irq < 0) {
		irq_domain_remove(domain);
		return irq;
	}

	ret = request_threaded_irq(irq, NULL, thread_fn, IRQF_ONESHOT, devname,
				   iommu);
	if (ret) {
		irq_domain_free_irqs(irq, 1);
		irq_domain_remove(domain);
		return ret;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static int iommu_setup_intcapxt(struct amd_iommu *iommu)
{
	int ret;

	snprintf(iommu->evt_irq_name, sizeof(iommu->evt_irq_name),
		 "AMD-Vi%d-Evt", iommu->index);
	ret = __iommu_setup_intcapxt(iommu, iommu->evt_irq_name,
				     MMIO_INTCAPXT_EVT_OFFSET,
				     amd_iommu_int_thread_evtlog);
	if (ret)
		return ret;

	snprintf(iommu->ppr_irq_name, sizeof(iommu->ppr_irq_name),
		 "AMD-Vi%d-PPR", iommu->index);
	ret = __iommu_setup_intcapxt(iommu, iommu->ppr_irq_name,
				     MMIO_INTCAPXT_PPR_OFFSET,
				     amd_iommu_int_thread_pprlog);
	if (ret)
		return ret;

#ifdef CONFIG_IRQ_REMAP
	snprintf(iommu->ga_irq_name, sizeof(iommu->ga_irq_name),
		 "AMD-Vi%d-GA", iommu->index);
	ret = __iommu_setup_intcapxt(iommu, iommu->ga_irq_name,
				     MMIO_INTCAPXT_GALOG_OFFSET,
				     amd_iommu_int_thread_galog);
#endif

	return ret;
}
/* …（同文件无关代码省略）… */
static int iommu_init_irq(struct amd_iommu *iommu)
{
	int ret;

	if (iommu->int_enabled)
		goto enable_faults;

	if (amd_iommu_xt_mode == IRQ_REMAP_X2APIC_MODE)
		ret = iommu_setup_intcapxt(iommu);
	else if (iommu->dev->msi_cap)
		ret = iommu_setup_msi(iommu);
	else
		ret = -ENODEV;

	if (ret)
		return ret;

	iommu->int_enabled = true;
enable_faults:

	if (amd_iommu_xt_mode == IRQ_REMAP_X2APIC_MODE)
		iommu_feature_enable(iommu, CONTROL_INTCAPXT_EN);

	iommu_feature_enable(iommu, CONTROL_EVT_INT_EN);

	return 0;
}
/* …（同文件无关代码省略）… */
static int __init init_unity_map_range(struct ivmd_header *m,
				       struct acpi_table_header *ivrs_base)
{
	struct unity_map_entry *e = NULL;
	struct amd_iommu_pci_seg *pci_seg;
	char *s;

	pci_seg = get_pci_segment(m->pci_seg, ivrs_base);
	if (pci_seg == NULL)
		return -ENOMEM;

	e = kzalloc_obj(*e);
	if (e == NULL)
		return -ENOMEM;

	switch (m->type) {
	default:
		kfree(e);
		return 0;
	case ACPI_IVMD_TYPE:
		s = "IVMD_TYPEi\t\t\t";
		e->devid_start = e->devid_end = m->devid;
		break;
	case ACPI_IVMD_TYPE_ALL:
		s = "IVMD_TYPE_ALL\t\t";
		e->devid_start = 0;
		e->devid_end = pci_seg->last_bdf;
		break;
	case ACPI_IVMD_TYPE_RANGE:
		s = "IVMD_TYPE_RANGE\t\t";
		e->devid_start = m->devid;
		e->devid_end = m->aux;
		break;
	}
	e->address_start = PAGE_ALIGN(m->range_start);
	e->address_end = e->address_start + PAGE_ALIGN(m->range_length);
	e->prot = m->flags >> 1;

	/*
	 * Treat per-device exclusion ranges as r/w unity-mapped regions
	 * since some buggy BIOSes might lead to the overwritten exclusion
	 * range (exclusion_start and exclusion_length members). This
	 * happens when there are multiple exclusion ranges (IVMD entries)
	 * defined in ACPI table.
	 */
	if (m->flags & IVMD_FLAG_EXCL_RANGE)
		e->prot = (IVMD_FLAG_IW | IVMD_FLAG_IR) >> 1;

	DUMP_printk("%s devid_start: %04x:%02x:%02x.%x devid_end: "
		    "%04x:%02x:%02x.%x range_start: %016llx range_end: %016llx"
		    " flags: %x\n", s, m->pci_seg,
		    PCI_BUS_NUM(e->devid_start), PCI_SLOT(e->devid_start),
		    PCI_FUNC(e->devid_start), m->pci_seg,
		    PCI_BUS_NUM(e->devid_end),
		    PCI_SLOT(e->devid_end), PCI_FUNC(e->devid_end),
		    e->address_start, e->address_end, m->flags);

	list_add_tail(&e->list, &pci_seg->unity_map);

	return 0;
}

/* iterates over all memory definitions we find in the ACPI table */
static int __init init_memory_definitions(struct acpi_table_header *table)
{
/* …（同文件无关代码省略）… */

	while (p < end) {
		m = (struct ivmd_header *)p;
		if (m->flags & (IVMD_FLAG_UNITY_MAP | IVMD_FLAG_EXCL_RANGE))
			init_unity_map_range(m, table);

		p += m->length;
	}

	return 0;
/* …（同文件无关代码省略）… */
static void iommu_init_flags(struct amd_iommu *iommu)
{
	iommu->acpi_flags & IVHD_FLAG_HT_TUN_EN_MASK ?
		iommu_feature_enable(iommu, CONTROL_HT_TUN_EN) :
		iommu_feature_disable(iommu, CONTROL_HT_TUN_EN);

	iommu->acpi_flags & IVHD_FLAG_PASSPW_EN_MASK ?
		iommu_feature_enable(iommu, CONTROL_PASSPW_EN) :
		iommu_feature_disable(iommu, CONTROL_PASSPW_EN);

	iommu->acpi_flags & IVHD_FLAG_RESPASSPW_EN_MASK ?
		iommu_feature_enable(iommu, CONTROL_RESPASSPW_EN) :
		iommu_feature_disable(iommu, CONTROL_RESPASSPW_EN);

	iommu->acpi_flags & IVHD_FLAG_ISOC_EN_MASK ?
		iommu_feature_enable(iommu, CONTROL_ISOC_EN) :
		iommu_feature_disable(iommu, CONTROL_ISOC_EN);

	/*
	 * make IOMMU memory accesses cache coherent
	 */
	iommu_feature_enable(iommu, CONTROL_COHERENT_EN);

	/* Set IOTLB invalidation timeout to 1s */
	iommu_feature_set(iommu, CTRL_INV_TO_1S, CTRL_INV_TO_MASK, CONTROL_INV_TIMEOUT);

	/* Enable Enhanced Peripheral Page Request Handling */
	if (check_feature(FEATURE_EPHSUP))
		iommu_feature_enable(iommu, CONTROL_EPH_EN);
}
/* …（同文件无关代码省略）… */
static void iommu_enable_ga(struct amd_iommu *iommu)
{
#ifdef CONFIG_IRQ_REMAP
	switch (amd_iommu_guest_ir) {
	case AMD_IOMMU_GUEST_IR_VAPIC:
	case AMD_IOMMU_GUEST_IR_LEGACY_GA:
		iommu_feature_enable(iommu, CONTROL_GA_EN);
		iommu->irte_ops = &irte_128_ops;
		break;
	default:
		iommu->irte_ops = &irte_32_ops;
		break;
	}
#endif
}
/* …（同文件无关代码省略）… */
static void iommu_enable_irtcachedis(struct amd_iommu *iommu)
{
	u64 ctrl;

	if (!amd_iommu_irtcachedis)
		return;

	/*
	 * Note:
	 * The support for IRTCacheDis feature is dertermined by
	 * checking if the bit is writable.
	 */
	iommu_feature_enable(iommu, CONTROL_IRTCACHEDIS);
	ctrl = readq(iommu->mmio_base +  MMIO_CONTROL_OFFSET);
	ctrl &= (1ULL << CONTROL_IRTCACHEDIS);
	if (ctrl)
		iommu->irtcachedis_enabled = true;
	pr_info("iommu%d (%#06x) : IRT cache is %s\n",
		iommu->index, iommu->devid,
		iommu->irtcachedis_enabled ? "disabled" : "enabled");
}
/* …（同文件无关代码省略）… */
static void iommu_enable_2k_int(struct amd_iommu *iommu)
{
	if (!FEATURE_NUM_INT_REMAP_SUP_2K(amd_iommu_efr2))
		return;

	iommu_feature_set(iommu,
			  CONTROL_NUM_INT_REMAP_MODE_2K,
			  CONTROL_NUM_INT_REMAP_MODE_MASK,
			  CONTROL_NUM_INT_REMAP_MODE);
}
/* …（同文件无关代码省略）… */
static void early_enable_iommu(struct amd_iommu *iommu)
{
	iommu_disable(iommu);
	iommu_init_flags(iommu);
	iommu_set_device_table(iommu);
	iommu_enable_command_buffer(iommu);
	iommu_enable_gt(iommu);
	iommu_enable_ga(iommu);
	iommu_enable_xt(iommu);
	iommu_enable_irtcachedis(iommu);
	iommu_enable_2k_int(iommu);
	iommu_enable(iommu);
	amd_iommu_flush_all_caches(iommu);
}
/* …（同文件无关代码省略）… */
static void enable_iommus_ppr(void)
{
	struct amd_iommu *iommu;

	if (!amd_iommu_gt_ppr_supported())
		return;

	for_each_iommu(iommu)
		amd_iommu_enable_ppr_log(iommu);
}
/* …（同文件无关代码省略）… */
static void enable_iommus_vapic(void)
{
#ifdef CONFIG_IRQ_REMAP
	u32 status, i;
	struct amd_iommu *iommu;

	for_each_iommu(iommu) {
		/*
		 * Disable GALog if already running. It could have been enabled
		 * in the previous boot before kdump.
		 */
		status = readl(iommu->mmio_base + MMIO_STATUS_OFFSET);
		if (!(status & MMIO_STATUS_GALOG_RUN_MASK))
			continue;

		iommu_feature_disable(iommu, CONTROL_GALOG_EN);
		iommu_feature_disable(iommu, CONTROL_GAINT_EN);

		/*
		 * Need to set and poll check the GALOGRun bit to zero before
		 * we can set/ modify GA Log registers safely.
		 */
		for (i = 0; i < MMIO_STATUS_TIMEOUT; ++i) {
			status = readl(iommu->mmio_base + MMIO_STATUS_OFFSET);
			if (!(status & MMIO_STATUS_GALOG_RUN_MASK))
				break;
			udelay(10);
		}

		if (WARN_ON(i >= MMIO_STATUS_TIMEOUT))
			return;
	}

	if (!AMD_IOMMU_GUEST_IR_VAPIC(amd_iommu_guest_ir))
		return;

	/* Enabling GAM and SNPAVIC support */
	for_each_iommu(iommu) {
		if (!iommu_ga_log_enable(iommu))
			return;

		iommu_feature_enable(iommu, CONTROL_GAM_EN);
		if (amd_iommu_snp_en)
			iommu_feature_enable(iommu, CONTROL_SNPAVIC_EN);
	}

	amd_iommu_irq_ops.capability |= (1 << IRQ_POSTING_CAP);
	pr_info("Virtual APIC enabled\n");
#endif
}

static int clear_perfopt_all(void)
{
	struct amd_iommu *iommu;
	int err, ret = 0;

	for_each_iommu(iommu) {
		err = amd_iommu_perfopt_clear(iommu);
		if (err)
			ret = err;
	}

	return ret;
}

static int restore_perfopt_all(void)
{
	struct amd_iommu *iommu;
	int err, ret = 0;

	for_each_iommu(iommu) {
		err = amd_iommu_perfopt_restore(iommu);
		if (err)
			ret = err;
	}

	return ret;
}

static void disable_iommus(void)
{
	struct amd_iommu *iommu;

	/*
	 * PerfOpt is an optional performance bit, so a failure to clear it must
	 * not skip the mandatory disable below. This also runs from the void
	 * amd_iommu_disable() shutdown/kexec path, which cannot report an error.
	 */
	if (clear_perfopt_all())
		pr_err("Failed to clear PerfOpt while disabling IOMMUs\n");

	for_each_iommu(iommu)
		iommu_disable(iommu);

/* …（同文件无关代码省略）… */
	for_each_iommu(iommu)
		early_enable_iommu(iommu);

	/* early_enable_iommu() cleared PERF_OPT_EN; re-assert it from the refcount. */
	if (restore_perfopt_all())
		pr_err("Failed to restore PerfOpt after IOMMU resume\n");

	iommu_enable_event_buffer();
	amd_iommu_enable_interrupts();
/* …（同文件无关代码省略）… */
static int amd_iommu_enable_interrupts(void)
{
	struct amd_iommu *iommu;
	int ret = 0;

	for_each_iommu(iommu) {
		ret = iommu_init_irq(iommu);
		if (ret)
			goto out;
	}

	/*
	 * Interrupt handler is ready to process interrupts. Enable
	 * PPR and GA log interrupt for all IOMMUs.
	 */
	enable_iommus_vapic();
	enable_iommus_ppr();

out:
	return ret;
}
/* …（同文件无关代码省略）… */
	return;

disable_snp:
	cc_platform_clear(CC_ATTR_HOST_SEV_SNP);
#endif
}
