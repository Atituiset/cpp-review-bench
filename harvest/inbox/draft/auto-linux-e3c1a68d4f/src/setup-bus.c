// AUTO-DRAFT from torvalds/linux PR #fddfc3ec31799a932bb92f1b8a84cb3d1f963be9
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define PCI_RES_TYPE_MASK \
	(IORESOURCE_IO | IORESOURCE_MEM | IORESOURCE_PREFETCH |\
	 IORESOURCE_MEM_64)
/* …（同文件无关代码省略）… */
struct pci_dev_resource {
	struct list_head list;
	struct resource *res;
	struct pci_dev *dev;
	resource_size_t start;
	resource_size_t end;
	resource_size_t add_size;
	resource_size_t min_align;
	unsigned long flags;
};
/* …（同文件无关代码省略）… */
static void pci_dev_res_free_list(struct list_head *head)
{
	struct pci_dev_resource *dev_res, *tmp;

	list_for_each_entry_safe(dev_res, tmp, head, list) {
		list_del(&dev_res->list);
		kfree(dev_res);
	}
}
/* …（同文件无关代码省略）… */
int pci_dev_res_add_to_list(struct list_head *head, struct pci_dev *dev,
			    struct resource *res, resource_size_t add_size,
			    resource_size_t min_align)
{
	struct pci_dev_resource *tmp;

	tmp = kzalloc_obj(*tmp);
	if (!tmp)
		return -ENOMEM;

	tmp->res = res;
	tmp->dev = dev;
	tmp->start = res->start;
	tmp->end = res->end;
	tmp->flags = res->flags;
	tmp->add_size = add_size;
	tmp->min_align = min_align;

	list_add(&tmp->list, head);

	return 0;
}
/* …（同文件无关代码省略）… */
static void pci_dev_res_remove_from_list(struct list_head *head,
					 struct resource *res)
{
	struct pci_dev_resource *dev_res, *tmp;

	list_for_each_entry_safe(dev_res, tmp, head, list) {
		if (dev_res->res == res) {
			list_del(&dev_res->list);
			kfree(dev_res);
			break;
		}
	}
}
/* …（同文件无关代码省略）… */
static struct pci_dev_resource *res_to_dev_res(struct list_head *head,
					       struct resource *res)
{
	struct pci_dev_resource *dev_res;

	list_for_each_entry(dev_res, head, list) {
		if (dev_res->res == res)
			return dev_res;
	}

	return NULL;
}
/* …（同文件无关代码省略）… */
static resource_size_t get_res_add_size(struct list_head *head,
					struct resource *res)
{
	struct pci_dev_resource *dev_res;

	dev_res = res_to_dev_res(head, res);
	return dev_res ? dev_res->add_size : 0;
}
/* …（同文件无关代码省略）… */
static void pci_dev_res_restore(struct pci_dev_resource *dev_res)
{
	struct resource *res = dev_res->res;
	struct pci_dev *dev = dev_res->dev;
	int idx = pci_resource_num(dev, res);
	const char *res_name = pci_resource_name(dev, idx);

	if (WARN_ON_ONCE(resource_assigned(res)))
		return;

	res->start = dev_res->start;
	res->end = dev_res->end;
	res->flags = dev_res->flags;

	pci_dbg(dev, "%s %pR: resource restored\n", res_name, res);
}
/* …（同文件无关代码省略）… */
static struct resource *find_bus_resource_of_type(struct pci_bus *bus,
						  unsigned long type_mask,
						  unsigned long type)
{
	struct resource *r, *r_assigned = NULL;

	pci_bus_for_each_resource(bus, r) {
		if (!r || r == &ioport_resource || r == &iomem_resource)
			continue;

		if ((r->flags & type_mask) != type)
			continue;

		if (!resource_assigned(r))
			return r;
		if (!r_assigned)
			r_assigned = r;
	}
	return r_assigned;
}
/* …（同文件无关代码省略）… */
static struct resource *pbus_select_window_for_type(struct pci_bus *bus,
						    unsigned long type)
{
	int iores_type = type & IORESOURCE_TYPE_BITS;	/* w/o 64bit & pref */
	struct resource *mmio, *mmio_pref, *win;

	type &= PCI_RES_TYPE_MASK;			/* with 64bit & pref */

	if ((iores_type != IORESOURCE_IO) && (iores_type != IORESOURCE_MEM))
		return NULL;

	if (pci_is_root_bus(bus)) {
		win = find_bus_resource_of_type(bus, type, type);
		if (win)
			return win;

		type &= ~IORESOURCE_MEM_64;
		win = find_bus_resource_of_type(bus, type, type);
		if (win)
			return win;

		type &= ~IORESOURCE_PREFETCH;
		return find_bus_resource_of_type(bus, type, type);
	}

	switch (iores_type) {
	case IORESOURCE_IO:
		win = pci_bus_resource_n(bus, PCI_BUS_BRIDGE_IO_WINDOW);
		if (win && (win->flags & IORESOURCE_IO))
			return win;
		return NULL;

	case IORESOURCE_MEM:
		mmio = pci_bus_resource_n(bus, PCI_BUS_BRIDGE_MEM_WINDOW);
		mmio_pref = pci_bus_resource_n(bus, PCI_BUS_BRIDGE_PREF_MEM_WINDOW);

		if (mmio && !(mmio->flags & IORESOURCE_MEM))
			mmio = NULL;
		if (mmio_pref && !(mmio_pref->flags & IORESOURCE_MEM))
			mmio_pref = NULL;

		if (!(type & IORESOURCE_PREFETCH) || !mmio_pref)
			return mmio;

		if ((type & IORESOURCE_MEM_64) ||
		    !(mmio_pref->flags & IORESOURCE_MEM_64))
			return mmio_pref;

		return mmio;
	default:
		return NULL;
	}
}
/* …（同文件无关代码省略）… */
struct resource *pbus_select_window(struct pci_bus *bus,
				    const struct resource *res)
{
	if (resource_assigned(res))
		return res->parent;

	return pbus_select_window_for_type(bus, res->flags);
}
/* …（同文件无关代码省略）… */
static bool pdev_resources_assignable(struct pci_dev *dev)
{
	u16 class = dev->class >> 8, command;

	/* Don't touch classless devices or host bridges or IOAPICs */
	if (class == PCI_CLASS_NOT_DEFINED || class == PCI_CLASS_BRIDGE_HOST)
		return false;

	/* Don't touch IOAPIC devices already enabled by firmware */
	if (class == PCI_CLASS_SYSTEM_PIC) {
		pci_read_config_word(dev, PCI_COMMAND, &command);
		if (command & (PCI_COMMAND_IO | PCI_COMMAND_MEMORY))
			return false;
	}

	return true;
}
/* …（同文件无关代码省略）… */
static bool pdev_resource_assignable(struct pci_dev *dev, struct resource *res)
{
	int idx = pci_resource_num(dev, res);

	if (!res->flags)
		return false;

	if (pci_resource_is_bridge_win(idx) && res->flags & IORESOURCE_DISABLED)
		return false;

	return true;
}
/* …（同文件无关代码省略）… */
static bool pdev_resource_should_fit(struct pci_dev *dev, struct resource *res)
{
	if (resource_assigned(res))
		return false;

	if (res->flags & IORESOURCE_PCI_FIXED)
		return false;

	return pdev_resource_assignable(dev, res);
}
/* …（同文件无关代码省略）… */
static void pdev_sort_resources(struct pci_dev *dev, struct list_head *head)
{
	struct resource *r;
	int i;

	if (!pdev_resources_assignable(dev))
		return;

	pci_dev_for_each_resource(dev, r, i) {
		const char *r_name = pci_resource_name(dev, i);
		struct pci_dev_resource *dev_res, *tmp;
		resource_size_t r_align;
		struct list_head *n;

		if (!pdev_resource_should_fit(dev, r))
			continue;

		r_align = pci_resource_alignment(dev, r);
		if (!r_align) {
			pci_warn(dev, "%s %pR: alignment must not be zero\n",
				 r_name, r);
			continue;
		}

		tmp = kzalloc_obj(*tmp);
		if (!tmp)
			panic("%s: kzalloc() failed!\n", __func__);
		tmp->res = r;
		tmp->dev = dev;
		tmp->start = r->start;
		tmp->end = r->end;
		tmp->flags = r->flags;

		/* Fallback is smallest one or list is empty */
		n = head;
		list_for_each_entry(dev_res, head, list) {
			resource_size_t align;

			align = pci_resource_alignment(dev_res->dev,
							 dev_res->res);

			if (r_align > align) {
				n = &dev_res->list;
				break;
			}
		}
		/* Insert it just before n */
		list_add_tail(&tmp->list, n);
	}
}
/* …（同文件无关代码省略）… */
bool pci_resource_is_optional(const struct pci_dev *dev, int resno)
{
	const struct resource *res = pci_resource_n(dev, resno);

	if (pci_resource_is_iov(resno))
		return true;
	if (resno == PCI_ROM_RESOURCE && !(res->flags & IORESOURCE_ROM_ENABLE))
		return true;
	if (pci_resource_is_bridge_win(resno) && !resource_size(res))
		return true;

	return false;
}
/* …（同文件无关代码省略）… */
static void reset_resource(struct pci_dev *dev, struct resource *res)
{
	int idx = pci_resource_num(dev, res);
	const char *res_name = pci_resource_name(dev, idx);

	if (pci_resource_is_bridge_win(idx)) {
		res->flags |= IORESOURCE_UNSET;
		return;
	}

	pci_dbg(dev, "%s %pR: resetting resource\n", res_name, res);

	res->start = 0;
	res->end = 0;
	res->flags = 0;
}
/* …（同文件无关代码省略）… */
static void reassign_resources_sorted(struct list_head *realloc_head,
				      struct list_head *head)
{
	struct pci_dev_resource *add_res, *tmp;
	struct pci_dev *dev;
	struct resource *res;
	const char *res_name;
	resource_size_t add_size, align;
	int idx;

	list_for_each_entry_safe(add_res, tmp, realloc_head, list) {
		res = add_res->res;
		dev = add_res->dev;
		idx = pci_resource_num(dev, res);

		/* Skip this resource if not found in head list */
		if (!res_to_dev_res(head, res))
			continue;

		/*
		 * Skip resource that failed the earlier assignment and is
		 * not optional as it would just fail again.
		 */
		if (!resource_assigned(res) && resource_size(res) &&
		    !pci_resource_is_optional(dev, idx))
			goto out;

		res_name = pci_resource_name(dev, idx);
		add_size = add_res->add_size;
		align = add_res->min_align;
		if (!resource_assigned(res)) {
			resource_set_range(res, align,
					   resource_size(res) + add_size);
			if (pci_assign_resource(dev, idx)) {
				pci_dbg(dev,
					"%s %pR: ignoring failure in optional allocation\n",
					res_name, res);
			}
		} else if (add_size > 0 || !IS_ALIGNED(res->start, align)) {
			res->flags |= add_res->flags &
				 (IORESOURCE_STARTALIGN|IORESOURCE_SIZEALIGN);
			if (pci_reassign_resource(dev, idx, add_size, align))
				pci_info(dev, "%s %pR: failed to add optional %llx\n",
					 res_name, res,
					 (unsigned long long) add_size);
		}
out:
		list_del(&add_res->list);
		kfree(add_res);
	}
}
/* …（同文件无关代码省略）… */
static void assign_requested_resources_sorted(struct list_head *head,
					      struct list_head *fail_head,
					      bool optional)
{
	struct pci_dev_resource *dev_res;
	struct resource *res;
	struct pci_dev *dev;
	bool optional_res;
	int idx;

	list_for_each_entry(dev_res, head, list) {
		res = dev_res->res;
		dev = dev_res->dev;
		idx = pci_resource_num(dev, res);
		optional_res = pci_resource_is_optional(dev, idx);

		if (!resource_size(res))
			continue;

		if (!optional && optional_res)
			continue;

		if (pci_assign_resource(dev, idx)) {
			if (fail_head) {
				pci_dev_res_add_to_list(fail_head, dev, res,
							0 /* don't care */,
							0 /* don't care */);
			}
		}
	}
}
/* …（同文件无关代码省略）… */
static unsigned long pci_fail_res_type_mask(struct list_head *fail_head)
{
	struct pci_dev_resource *fail_res;
	unsigned long mask = 0;

	/* Check failed type */
	list_for_each_entry(fail_res, fail_head, list)
		mask |= fail_res->flags;

	/*
	 * One pref failed resource will set IORESOURCE_MEM, as we can
	 * allocate pref in non-pref range.  Will release all assigned
	 * non-pref sibling resources according to that bit.
	 */
	return mask & (IORESOURCE_IO | IORESOURCE_MEM | IORESOURCE_PREFETCH);
}
/* …（同文件无关代码省略）… */
static bool pci_need_to_release(unsigned long mask, struct resource *res)
{
	if (res->flags & IORESOURCE_IO)
		return !!(mask & IORESOURCE_IO);

	/* Check pref at first */
	if (res->flags & IORESOURCE_PREFETCH) {
		if (mask & IORESOURCE_PREFETCH)
			return true;
		/* Count pref if its parent is non-pref */
		else if ((mask & IORESOURCE_MEM) &&
			 !(res->parent->flags & IORESOURCE_PREFETCH))
			return true;
		else
			return false;
	}

	if (res->flags & IORESOURCE_MEM)
		return !!(mask & IORESOURCE_MEM);

	return false;	/* Should not get here */
}
/* …（同文件无关代码省略）… */
static bool pci_required_resource_failed(struct list_head *fail_head,
					 unsigned long type)
{
	struct pci_dev_resource *fail_res;

	type &= PCI_RES_TYPE_MASK;

	list_for_each_entry(fail_res, fail_head, list) {
		int idx = pci_resource_num(fail_res->dev, fail_res->res);

		if (type && (fail_res->flags & PCI_RES_TYPE_MASK) != type)
			continue;

		if (!pci_resource_is_optional(fail_res->dev, idx))
			return true;
	}
	return false;
}
/* …（同文件无关代码省略）… */
static void __assign_resources_sorted(struct list_head *head,
				      struct list_head *realloc_head,
				      struct list_head *fail_head)
{
	/*
	 * Should not assign requested resources at first.  They could be
	 * adjacent, so later reassign can not reallocate them one by one in
	 * parent resource window.
	 *
	 * Try to assign required and any optional resources at beginning
	 * (add_size included). If all required resources were successfully
	 * assigned, get out early. If could not do that, we still try to
	 * assign required at first, then try to reassign some optional
	 * resources.
	 *
	 * Separate three resource type checking if we need to release
	 * assigned resource after requested + add_size try.
	 *
	 *	1. If IO port assignment fails, will release assigned IO
	 *	   port.
	 *	2. If pref MMIO assignment fails, release assigned pref
	 *	   MMIO.  If assigned pref MMIO's parent is non-pref MMIO
	 *	   and non-pref MMIO assignment fails, will release that
	 *	   assigned pref MMIO.
	 *	3. If non-pref MMIO assignment fails or pref MMIO
	 *	   assignment fails, will release assigned non-pref MMIO.
	 */
	LIST_HEAD(save_head);
	LIST_HEAD(local_fail_head);
	LIST_HEAD(dummy_head);
	struct pci_dev_resource *save_res;
	struct pci_dev_resource *dev_res, *tmp_res, *dev_res2, *addsize_res;
	struct resource *res;
	struct pci_dev *dev;
	unsigned long fail_type;
	resource_size_t align;

	if (!realloc_head)
		realloc_head = &dummy_head;

	/* Check if optional add_size is there */
	if (list_empty(realloc_head))
		goto assign;

	/* Save original start, end, flags etc at first */
	list_for_each_entry(dev_res, head, list) {
		if (pci_dev_res_add_to_list(&save_head, dev_res->dev,
					    dev_res->res, 0, 0)) {
			pci_dev_res_free_list(&save_head);
			goto assign;
		}
	}

	/* Update res in head list with add_size in realloc_head list */
	list_for_each_entry_safe(dev_res, tmp_res, head, list) {
		res = dev_res->res;

		addsize_res = res_to_dev_res(realloc_head, res);
		if (!addsize_res)
			continue;

		res->end += addsize_res->add_size;
		/*
		 * There are two kinds of additional resources in the list:
		 * 1. bridge resource  -- IORESOURCE_STARTALIGN
		 * 2. SR-IOV resource  -- IORESOURCE_SIZEALIGN
		 * Here just fix the additional alignment for bridge
		 */
		if (!(res->flags & IORESOURCE_STARTALIGN))
			continue;

		if (addsize_res->min_align <= res->start)
			continue;
		/*
		 * The "head" list is sorted by alignment so resources with
		 * bigger alignment will be assigned first.  After we
		 * change the alignment of a dev_res in "head" list, we
		 * need to reorder the list by alignment to make it
		 * consistent.
		 */
		resource_set_range(res, addsize_res->min_align,
				   resource_size(res));

		list_for_each_entry(dev_res2, head, list) {
			align = pci_resource_alignment(dev_res2->dev,
						       dev_res2->res);
			if (addsize_res->min_align > align) {
				list_move_tail(&dev_res->list, &dev_res2->list);
				break;
			}
		}

	}

assign:
	assign_requested_resources_sorted(head, &local_fail_head, true);

	/* All non-optional resources assigned? */
	if (list_empty(&local_fail_head)) {
		/* Remove head list from realloc_head list */
		list_for_each_entry(dev_res, head, list)
			pci_dev_res_remove_from_list(realloc_head,
						     dev_res->res);
		pci_dev_res_free_list(&save_head);
		goto out;
	}

	/* Without realloc_head and only optional fails, nothing more to do. */
	if (!pci_required_resource_failed(&local_fail_head, 0) &&
	    list_empty(realloc_head)) {
		list_for_each_entry(save_res, &save_head, list) {
			struct resource *res = save_res->res;

			if (resource_assigned(res))
				continue;

			pci_dev_res_restore(save_res);
		}
		pci_dev_res_free_list(&local_fail_head);
		pci_dev_res_free_list(&save_head);
		goto out;
	}

	/* Check failed type */
	fail_type = pci_fail_res_type_mask(&local_fail_head);
	/* Remove not need to be released assigned res from head list etc */
	list_for_each_entry_safe(dev_res, tmp_res, head, list) {
		res = dev_res->res;

		if (resource_assigned(res) &&
		    !pci_need_to_release(fail_type, res)) {
			/* Remove it from realloc_head list */
			pci_dev_res_remove_from_list(realloc_head, res);
			pci_dev_res_remove_from_list(&save_head, res);
			list_del(&dev_res->list);
			kfree(dev_res);
		}
	}

	pci_dev_res_free_list(&local_fail_head);
	/* Release assigned resource */
	list_for_each_entry(dev_res, head, list) {
		res = dev_res->res;
		dev = dev_res->dev;

		pci_release_resource(dev, pci_resource_num(dev, res));
		pci_dev_res_restore(dev_res);
	}
	/* Restore start/end/flags from saved list */
	list_for_each_entry(save_res, &save_head, list)
		pci_dev_res_restore(save_res);
	pci_dev_res_free_list(&save_head);

	/* Satisfy the must-have resource requests */
	assign_requested_resources_sorted(head, NULL, false);

	/* Try to satisfy any additional optional resource requests */
	if (!list_empty(realloc_head))
		reassign_resources_sorted(realloc_head, head);

out:
	/* Reset any failed resource, cannot use fail_head as it can be NULL. */
	list_for_each_entry(dev_res, head, list) {
		res = dev_res->res;
		dev = dev_res->dev;

		if (resource_assigned(res))
			continue;

		if (fail_head) {
			pci_dev_res_add_to_list(fail_head, dev, res,
						0 /* don't care */,
						0 /* don't care */);
		}

		reset_resource(dev, res);
	}

	pci_dev_res_free_list(head);
}
/* …（同文件无关代码省略）… */
static void pdev_assign_resources_sorted(struct pci_dev *dev,
					 struct list_head *add_list,
					 struct list_head *fail_head)
{
	LIST_HEAD(head);

	pdev_sort_resources(dev, &head);
	__assign_resources_sorted(&head, add_list, fail_head);

}
/* …（同文件无关代码省略）… */
static void pbus_assign_resources_sorted(const struct pci_bus *bus,
					 struct list_head *realloc_head,
					 struct list_head *fail_head)
{
	struct pci_dev *dev;
	LIST_HEAD(head);

	list_for_each_entry(dev, &bus->devices, bus_list)
		pdev_sort_resources(dev, &head);

	__assign_resources_sorted(&head, realloc_head, fail_head);
}
/* …（同文件无关代码省略）… */
static void pci_setup_bridge_io(struct pci_dev *bridge)
{
	struct resource *res;
	const char *res_name;
	struct pci_bus_region region;
	unsigned long io_mask;
	u8 io_base_lo, io_limit_lo;
	u16 l;
	u32 io_upper16;

	io_mask = PCI_IO_RANGE_MASK;
	if (bridge->io_window_1k)
		io_mask = PCI_IO_1K_RANGE_MASK;

	/* Set up the top and bottom of the PCI I/O segment for this bus */
	res = &bridge->resource[PCI_BRIDGE_IO_WINDOW];
	res_name = pci_resource_name(bridge, PCI_BRIDGE_IO_WINDOW);
	pcibios_resource_to_bus(bridge->bus, &region, res);
	if (resource_assigned(res) && res->flags & IORESOURCE_IO) {
		pci_read_config_word(bridge, PCI_IO_BASE, &l);
		io_base_lo = (region.start >> 8) & io_mask;
		io_limit_lo = (region.end >> 8) & io_mask;
		l = ((u16) io_limit_lo << 8) | io_base_lo;
		/* Set up upper 16 bits of I/O base/limit */
		io_upper16 = (region.end & 0xffff0000) | (region.start >> 16);
		pci_info(bridge, "  %s %pR\n", res_name, res);
	} else {
		/* Clear upper 16 bits of I/O base/limit */
		io_upper16 = 0;
		l = 0x00f0;
	}
	/* Temporarily disable the I/O range before updating PCI_IO_BASE */
	pci_write_config_dword(bridge, PCI_IO_BASE_UPPER16, 0x0000ffff);
	/* Update lower 16 bits of I/O base/limit */
	pci_write_config_word(bridge, PCI_IO_BASE, l);
	/* Update upper 16 bits of I/O base/limit */
	pci_write_config_dword(bridge, PCI_IO_BASE_UPPER16, io_upper16);
}
/* …（同文件无关代码省略）… */
static void pci_setup_bridge_mmio(struct pci_dev *bridge)
{
	struct resource *res;
	const char *res_name;
	struct pci_bus_region region;
	u32 l;

	/* Set up the top and bottom of the PCI Memory segment for this bus */
	res = &bridge->resource[PCI_BRIDGE_MEM_WINDOW];
	res_name = pci_resource_name(bridge, PCI_BRIDGE_MEM_WINDOW);
	pcibios_resource_to_bus(bridge->bus, &region, res);
	if (resource_assigned(res) && res->flags & IORESOURCE_MEM) {
		l = (region.start >> 16) & 0xfff0;
		l |= region.end & 0xfff00000;
		pci_info(bridge, "  %s %pR\n", res_name, res);
	} else {
		l = 0x0000fff0;
	}
	pci_write_config_dword(bridge, PCI_MEMORY_BASE, l);
}
/* …（同文件无关代码省略）… */
static void pci_setup_bridge_mmio_pref(struct pci_dev *bridge)
{
	struct resource *res;
	const char *res_name;
	struct pci_bus_region region;
	u32 l, bu, lu;

	/*
	 * Clear out the upper 32 bits of PREF limit.  If
	 * PCI_PREF_BASE_UPPER32 was non-zero, this temporarily disables
	 * PREF range, which is ok.
	 */
	pci_write_config_dword(bridge, PCI_PREF_LIMIT_UPPER32, 0);

	/* Set up PREF base/limit */
	bu = lu = 0;
	res = &bridge->resource[PCI_BRIDGE_PREF_MEM_WINDOW];
	res_name = pci_resource_name(bridge, PCI_BRIDGE_PREF_MEM_WINDOW);
	pcibios_resource_to_bus(bridge->bus, &region, res);
	if (resource_assigned(res) && res->flags & IORESOURCE_PREFETCH) {
		l = (region.start >> 16) & 0xfff0;
		l |= region.end & 0xfff00000;
		if (res->flags & IORESOURCE_MEM_64) {
			bu = upper_32_bits(region.start);
			lu = upper_32_bits(region.end);
		}
		pci_info(bridge, "  %s %pR\n", res_name, res);
	} else {
		l = 0x0000fff0;
	}
	pci_write_config_dword(bridge, PCI_PREF_MEMORY_BASE, l);

	/* Set the upper 32 bits of PREF base & limit */
	pci_write_config_dword(bridge, PCI_PREF_BASE_UPPER32, bu);
	pci_write_config_dword(bridge, PCI_PREF_LIMIT_UPPER32, lu);
}
/* …（同文件无关代码省略）… */
static void __pci_setup_bridge(struct pci_bus *bus, unsigned long type)
{
	struct pci_dev *bridge = bus->self;

	pci_info(bridge, "PCI bridge to %pR\n", &bus->busn_res);

	if (type & IORESOURCE_IO)
		pci_setup_bridge_io(bridge);

	if (type & IORESOURCE_MEM)
		pci_setup_bridge_mmio(bridge);

	if (type & IORESOURCE_PREFETCH)
		pci_setup_bridge_mmio_pref(bridge);

	pci_write_config_word(bridge, PCI_BRIDGE_CONTROL, bus->bridge_ctl);
}
/* …（同文件无关代码省略）… */
void __weak pcibios_setup_bridge(struct pci_bus *bus, unsigned long type)
{
}
/* …（同文件无关代码省略）… */
static void pci_setup_bridge(struct pci_bus *bus)
{
	unsigned long type = IORESOURCE_IO | IORESOURCE_MEM |
				  IORESOURCE_PREFETCH;

	pcibios_setup_bridge(bus, type);
	__pci_setup_bridge(bus, type);
}
/* …（同文件无关代码省略）… */
static void pci_bridge_check_ranges(struct pci_bus *bus)
{
	struct pci_dev *bridge = bus->self;
	struct resource *b_res;

	b_res = &bridge->resource[PCI_BRIDGE_MEM_WINDOW];
	b_res->flags |= IORESOURCE_MEM;

	if (bridge->io_window) {
		b_res = &bridge->resource[PCI_BRIDGE_IO_WINDOW];
		b_res->flags |= IORESOURCE_IO;
	}

	if (bridge->pref_window) {
		b_res = &bridge->resource[PCI_BRIDGE_PREF_MEM_WINDOW];
		b_res->flags |= IORESOURCE_MEM | IORESOURCE_PREFETCH;
		if (bridge->pref_64_window) {
			b_res->flags |= IORESOURCE_MEM_64 |
					PCI_PREF_RANGE_TYPE_64;
		}
	}
}
/* …（同文件无关代码省略）… */
static resource_size_t calculate_iosize(resource_size_t size,
					resource_size_t min_size,
					resource_size_t size1,
					resource_size_t add_size,
					resource_size_t children_add_size,
					resource_size_t old_size,
					resource_size_t align)
{
	if (size < min_size)
		size = min_size;
	if (old_size == 1)
		old_size = 0;
	/*
	 * To be fixed in 2.5: we should have sort of HAVE_ISA flag in the
	 * struct pci_bus.
	 */
#if defined(CONFIG_ISA) || defined(CONFIG_EISA)
	size = (size & 0xff) + ((size & ~0xffUL) << 2);
#endif
	size = size + size1;

	size = max(size, add_size) + children_add_size;
	return ALIGN(max(size, old_size), align);
}
/* …（同文件无关代码省略）… */
static resource_size_t calculate_memsize(resource_size_t size,
					 resource_size_t min_size,
					 resource_size_t children_add_size,
					 resource_size_t align)
{
	size = max(size + children_add_size, min_size);
	return ALIGN(size, align);
}
/* …（同文件无关代码省略）… */
resource_size_t __weak pcibios_window_alignment(struct pci_bus *bus,
						unsigned long type)
{
	return 1;
}
/* …（同文件无关代码省略）… */
#define PCI_P2P_DEFAULT_MEM_ALIGN	SZ_1M
#define PCI_P2P_DEFAULT_IO_ALIGN	SZ_4K
#define PCI_P2P_DEFAULT_IO_ALIGN_1K	SZ_1K
/* …（同文件无关代码省略）… */
resource_size_t pci_min_window_alignment(struct pci_bus *bus, unsigned long type)
{
	resource_size_t align = 1, arch_align;

	if (type & IORESOURCE_MEM)
		align = PCI_P2P_DEFAULT_MEM_ALIGN;
	else if (type & IORESOURCE_IO) {
		/*
		 * Per spec, I/O windows are 4K-aligned, but some bridges have
		 * an extension to support 1K alignment.
		 */
		if (bus->self && bus->self->io_window_1k)
			align = PCI_P2P_DEFAULT_IO_ALIGN_1K;
		else
			align = PCI_P2P_DEFAULT_IO_ALIGN;
	}

	arch_align = pcibios_window_alignment(bus, type);
	return max(align, arch_align);
}
/* …（同文件无关代码省略）… */
static void pbus_size_io(struct pci_bus *bus, resource_size_t add_size,
			 struct list_head *realloc_head)
{
	struct pci_dev *dev;
	struct resource *b_res = pbus_select_window_for_type(bus, IORESOURCE_IO);
	resource_size_t size = 0, size0 = 0, size1 = 0;
	resource_size_t children_add_size = 0;
	resource_size_t min_align, align;

	if (!b_res)
		return;

	/* If resource is already assigned, nothing more to do */
	if (resource_assigned(b_res))
		return;

	min_align = pci_min_window_alignment(bus, IORESOURCE_IO);
	list_for_each_entry(dev, &bus->devices, bus_list) {
		struct resource *r;

		pci_dev_for_each_resource(dev, r) {
			unsigned long r_size;

			if (resource_assigned(r) || !(r->flags & IORESOURCE_IO))
				continue;

			if (!pdev_resource_assignable(dev, r))
				continue;

			r_size = resource_size(r);
			if (r_size < SZ_1K)
				/* Might be re-aligned for ISA */
				size += r_size;
			else
				size1 += r_size;

			align = pci_resource_alignment(dev, r);
			if (align > min_align)
				min_align = align;

			if (realloc_head)
				children_add_size += get_res_add_size(realloc_head, r);
		}
	}

	size0 = calculate_iosize(size, realloc_head ? 0 : add_size, size1, 0, 0,
			resource_size(b_res), min_align);

	if (size0)
		b_res->flags &= ~IORESOURCE_DISABLED;

	size1 = size0;
	if (realloc_head && (add_size > 0 || children_add_size > 0)) {
		size1 = calculate_iosize(size, 0, size1, add_size,
					 children_add_size, resource_size(b_res),
					 min_align);
	}

	if (!size0 && !size1) {
		if (bus->self && (b_res->start || b_res->end))
			pci_info(bus->self, "disabling bridge window %pR to %pR (unused)\n",
				 b_res, &bus->busn_res);
		b_res->flags |= IORESOURCE_DISABLED;
		return;
	}

	resource_set_range(b_res, min_align, size0);
	b_res->flags |= IORESOURCE_STARTALIGN;
	if (bus->self && size1 > size0 && realloc_head) {
		b_res->flags &= ~IORESOURCE_DISABLED;
		pci_dev_res_add_to_list(realloc_head, bus->self, b_res,
					size1 - size0, min_align);
		pci_info(bus->self, "bridge window %pR to %pR add_size %llx\n",
			 b_res, &bus->busn_res,
			 (unsigned long long) size1 - size0);
	}
}
/* …（同文件无关代码省略）… */
static resource_size_t calculate_head_align(resource_size_t *aligns,
					    int max_order)
{
	resource_size_t head_align = 1;
	resource_size_t remainder = 0;
	int order;

	/* Take the largest alignment as the starting point. */
	head_align <<= max_order + __ffs(SZ_1M);

	for (order = max_order - 1; order >= 0; order--) {
		resource_size_t align1 = 1;

		align1 <<= order + __ffs(SZ_1M);

		/*
		 * Account smaller resources with alignment < max_order that
		 * could be used to fill head room if alignment less than
		 * max_order is used.
		 */
		remainder += aligns[order];

		/*
		 * Test if head fill is enough to satisfy the alignment of
		 * the larger resources after reducing the alignment.
		 */
		while ((head_align > align1) && (remainder >= head_align / 2)) {
			head_align /= 2;
			remainder -= head_align;
		}
	}

	return head_align;
}
/* …（同文件无关代码省略）… */
static bool pbus_size_mem_optional(struct pci_dev *dev, int resno,
				   resource_size_t align,
				   struct list_head *realloc_head,
				   resource_size_t *add_align,
				   resource_size_t *children_add_size)
{
	struct resource *res = pci_resource_n(dev, resno);
	bool optional = pci_resource_is_optional(dev, resno);
	resource_size_t r_size = resource_size(res);
	struct pci_dev_resource *dev_res = NULL;

	if (!realloc_head)
		return false;

	/*
	 * Only bridges have optional sizes in realloc_head at this
	 * point. As res_to_dev_res() walks the entire realloc_head
	 * list, skip calling it when known unnecessary.
	 */
	if (pci_resource_is_bridge_win(resno)) {
		dev_res = res_to_dev_res(realloc_head, res);
		if (dev_res) {
			*children_add_size += dev_res->add_size;
			*add_align = max(*add_align, dev_res->min_align);
		}
	}

	if (!optional)
		return false;

	/*
	 * Put requested res to the optional list if not there yet (SR-IOV,
	 * disabled ROM). Bridge windows with an optional part are already
	 * on the list.
	 */
	if (!dev_res)
		pci_dev_res_add_to_list(realloc_head, dev, res, 0, align);
	*children_add_size += r_size;
	*add_align = max(align, *add_align);

	return true;
}
/* …（同文件无关代码省略）… */
static void pbus_size_mem(struct pci_bus *bus, struct resource *b_res,
			  resource_size_t add_size,
			  struct list_head *realloc_head)
{
	struct pci_dev *dev;
	resource_size_t min_align, win_align, align, size, size0, size1 = 0;
	resource_size_t aligns[28] = {}; /* Alignments from 1MB to 128TB */
	int order, max_order;
	resource_size_t children_add_size = 0;
	resource_size_t add_align = 0;

	if (!b_res)
		return;

	/* If resource is already assigned, nothing more to do */
	if (resource_assigned(b_res))
		return;

	max_order = 0;
	size = 0;

	list_for_each_entry(dev, &bus->devices, bus_list) {
		struct resource *r;
		int i;

		pci_dev_for_each_resource(dev, r, i) {
			const char *r_name = pci_resource_name(dev, i);
			resource_size_t r_size;

			if (!pdev_resources_assignable(dev) ||
			    !pdev_resource_should_fit(dev, r))
				continue;
			if (b_res != pbus_select_window(bus, r))
				continue;

			align = pci_resource_alignment(dev, r);
			/*
			 * aligns[0] is for 1MB (since bridge memory
			 * windows are always at least 1MB aligned), so
			 * keep "order" from being negative for smaller
			 * resources.
			 */
			order = max_t(int, __ffs(align) - __ffs(SZ_1M), 0);
			if (order >= ARRAY_SIZE(aligns)) {
				pci_warn(dev, "%s %pR: disabling; bad alignment %#llx\n",
					 r_name, r, (unsigned long long) align);
				r->flags = 0;
				continue;
			}

			if (pbus_size_mem_optional(dev, i, align,
						   realloc_head, &add_align,
						   &children_add_size))
				continue;

			r_size = resource_size(r);
			size += max(r_size, align);

			/*
			 * If resource's size is larger than its alignment,
			 * some configurations result in an unwanted gap in
			 * the head space that the larger resource cannot
			 * fill.
			 */
			if (r_size <= align)
				aligns[order] += align;
			if (order > max_order)
				max_order = order;
		}
	}

	win_align = pci_min_window_alignment(bus, b_res->flags);
	min_align = calculate_head_align(aligns, max_order);
	min_align = max(min_align, win_align);
	size0 = calculate_memsize(size, realloc_head ? 0 : add_size,
				  0, win_align);

	if (size0) {
		resource_set_range(b_res, min_align, size0);
		b_res->flags &= ~IORESOURCE_DISABLED;
	}

	if (realloc_head && (add_size > 0 || children_add_size > 0)) {
		add_align = max(min_align, add_align);
		size1 = calculate_memsize(size, add_size, children_add_size,
					  win_align);
	}

	if (!size0 && !size1) {
		if (bus->self && (b_res->start || b_res->end))
			pci_info(bus->self, "disabling bridge window %pR to %pR (unused)\n",
				 b_res, &bus->busn_res);
		b_res->flags |= IORESOURCE_DISABLED;
		return;
	}

	resource_set_range(b_res, min_align, size0);
	b_res->flags |= IORESOURCE_STARTALIGN;
	if (bus->self && realloc_head && (size1 > size0 || add_align > min_align)) {
		b_res->flags &= ~IORESOURCE_DISABLED;
		add_size = size1 > size0 ? size1 - size0 : 0;
		pci_dev_res_add_to_list(realloc_head, bus->self, b_res,
					add_size, add_align);
		pci_info(bus->self, "bridge window %pR to %pR add_size %llx add_align %llx\n",
			   b_res, &bus->busn_res,
			   (unsigned long long) add_size,
			   (unsigned long long) add_align);
	}
}
/* …（同文件无关代码省略）… */
void __pci_bus_size_bridges(struct pci_bus *bus, struct list_head *realloc_head)
{
	struct pci_dev *dev;
	resource_size_t additional_io_size = 0, additional_mmio_size = 0,
			additional_mmio_pref_size = 0;
	struct resource *b_res;
	struct pci_host_bridge *host;
	int hdr_type;

	list_for_each_entry(dev, &bus->devices, bus_list) {
		struct pci_bus *b = dev->subordinate;
		if (!b)
			continue;

		switch (dev->hdr_type) {
		case PCI_HEADER_TYPE_CARDBUS:
			if (pci_bus_size_cardbus_bridge(b, realloc_head))
				continue;
			break;

		case PCI_HEADER_TYPE_BRIDGE:
		default:
			__pci_bus_size_bridges(b, realloc_head);
			break;
		}
	}

	/* The root bus? */
	if (pci_is_root_bus(bus)) {
		host = to_pci_host_bridge(bus->bridge);
		if (!host->size_windows)
			return;
		hdr_type = -1;	/* Intentionally invalid - not a PCI device. */
	} else {
		hdr_type = bus->self->hdr_type;
	}

	switch (hdr_type) {
	case PCI_HEADER_TYPE_CARDBUS:
		/* Don't size CardBuses yet */
		break;

	case PCI_HEADER_TYPE_BRIDGE:
		pci_bridge_check_ranges(bus);
		if (bus->self->is_hotplug_bridge) {
			additional_io_size  = pci_hotplug_io_size;
			additional_mmio_size = pci_hotplug_mmio_size;
			additional_mmio_pref_size = pci_hotplug_mmio_pref_size;
		}
		fallthrough;
	default:
		pbus_size_io(bus, additional_io_size, realloc_head);

		b_res = pbus_select_window_for_type(bus, IORESOURCE_MEM |
							 IORESOURCE_PREFETCH |
							 IORESOURCE_MEM_64);
		if (b_res && (b_res->flags & IORESOURCE_PREFETCH)) {
			pbus_size_mem(bus, b_res, additional_mmio_pref_size,
				      realloc_head);
		}

		b_res = pbus_select_window_for_type(bus, IORESOURCE_MEM);
		if (b_res) {
			pbus_size_mem(bus, b_res, additional_mmio_size,
				      realloc_head);
		}
		break;
	}
}
/* …（同文件无关代码省略）… */
static void pdev_assign_fixed_resources(struct pci_dev *dev)
{
	struct resource *r;

	pci_dev_for_each_resource(dev, r) {
		struct pci_bus *b;

		if (resource_assigned(r) ||
		    !(r->flags & IORESOURCE_PCI_FIXED) ||
		    !(r->flags & (IORESOURCE_IO | IORESOURCE_MEM)))
			continue;

		b = dev->bus;
		while (b && !resource_assigned(r)) {
			assign_fixed_resource_on_bus(b, r);
			b = b->parent;
		}
	}
}
/* …（同文件无关代码省略）… */
static void __pci_bus_assign_resources_one(const struct pci_bus *bus,
					   struct list_head *add_list,
					   struct list_head *fail_head)
{
	struct pci_bus *b;
	struct pci_dev *dev;

	pbus_assign_resources_sorted(bus, add_list, fail_head);

	list_for_each_entry(dev, &bus->devices, bus_list) {
		pdev_assign_fixed_resources(dev);

		b = dev->subordinate;
		if (!b)
			continue;

		__pci_bus_assign_resources_one(b, add_list, fail_head);

		switch (dev->hdr_type) {
		case PCI_HEADER_TYPE_BRIDGE:
			if (!pci_is_enabled(dev))
				pci_setup_bridge(b);
			break;

		case PCI_HEADER_TYPE_CARDBUS:
			pci_setup_cardbus_bridge(b);
			break;

		default:
			pci_info(dev, "not setting up bridge for bus %04x:%02x\n",
				 pci_domain_nr(b), b->number);
			break;
		}
	}
}
/* …（同文件无关代码省略）… */
void __pci_bus_assign_resources(const struct pci_bus *bus,
				struct list_head *add_list,
				struct list_head *fail_head)
{
	__pci_bus_assign_resources_one(bus, add_list, fail_head);

	if (WARN_ON_ONCE(add_list && !list_empty(add_list)))
		pci_dev_res_free_list(add_list);
}
/* …（同文件无关代码省略）… */
static void __pci_bridge_assign_resources(struct pci_dev *bridge,
					  struct list_head *add_list,
					  struct list_head *fail_head)
{
	struct pci_bus *b;

	pdev_assign_resources_sorted(bridge, add_list, fail_head);

	b = bridge->subordinate;
	if (!b)
		return;

	__pci_bus_assign_resources(b, add_list, fail_head);

	switch (bridge->class >> 8) {
	case PCI_CLASS_BRIDGE_PCI:
		pci_setup_bridge(b);
		break;

	case PCI_CLASS_BRIDGE_CARDBUS:
		pci_setup_cardbus_bridge(b);
		break;

	default:
		pci_info(bridge, "not setting up bridge for bus %04x:%02x\n",
			 pci_domain_nr(b), b->number);
		break;
	}

	if (WARN_ON_ONCE(add_list && !list_empty(add_list)))
		pci_dev_res_free_list(add_list);
}
/* …（同文件无关代码省略）… */
static int pbus_reassign_bridge_resources(struct pci_bus *bus, struct resource *res,
					  struct list_head *saved)
{
	unsigned long type = res->flags;
	struct pci_dev_resource *dev_res;
	struct pci_dev *bridge = NULL;
	LIST_HEAD(add_list);
	LIST_HEAD(failed);
	unsigned int i;
	int ret = 0;

	while (!pci_is_root_bus(bus)) {
		bridge = bus->self;
		res = pbus_select_window(bus, res);
		if (!res)
			break;

		i = pci_resource_num(bridge, res);

		/* Ignore BARs which are still in use */
		if (!res->child) {
			ret = pci_dev_res_add_to_list(saved, bridge, res, 0, 0);
			if (ret)
				return ret;

			pci_release_resource(bridge, i);
		} else {
			const char *res_name = pci_resource_name(bridge, i);

			pci_warn(bridge,
				 "%s %pR: was not released (still contains assigned resources)\n",
				 res_name, res);
		}

		bus = bus->parent;
	}

	if (!bridge)
		return -ENOENT;

	__pci_bus_size_bridges(bridge->subordinate, &add_list);
	__pci_bridge_assign_resources(bridge, &add_list, &failed);

	if (!list_empty(&failed)) {
		if (pci_required_resource_failed(&failed, type))
			ret = -ENOSPC;
		pci_dev_res_free_list(&failed);
		if (ret)
			return ret;

		/* Only resources with unrelated types failed (again) */
	}

	list_for_each_entry(dev_res, saved, list) {
		struct pci_dev *dev = dev_res->dev;

		/* Skip the bridge we just assigned resources for */
		if (bridge == dev)
			continue;

		if (!dev->subordinate)
			continue;

		pci_setup_bridge(dev->subordinate);
	}

	return 0;
}
/* …（同文件无关代码省略）… */
	struct resource *res = pci_resource_n(pdev, resno);
	struct pci_dev_resource *dev_res;
	struct pci_bus *bus = pdev->bus;
	struct resource *b_win, *r;
	LIST_HEAD(saved);
	unsigned int i;
/* …（同文件无关代码省略）… */
	if (ret)
		return ret;

	pci_dev_for_each_resource(pdev, r, i) {
		if (i >= PCI_BRIDGE_RESOURCES)
			break;
/* …（同文件无关代码省略）… */

	pci_resize_resource_set_size(pdev, resno, size);

	if (!bus->self)
		goto out;

	down_read(&pci_bus_sem);
	ret = pbus_reassign_bridge_resources(bus, res, &saved);
	if (ret)
		goto restore;

out:
	up_read(&pci_bus_sem);
