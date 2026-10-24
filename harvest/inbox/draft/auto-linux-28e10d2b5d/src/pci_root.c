// AUTO-DRAFT from torvalds/linux PR #17bb0e2f6c8043f70a693a5f3797471cbed354c8
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static void acpi_pci_root_validate_resources(struct device *dev,
					     struct list_head *resources,
					     unsigned long type)
{
	LIST_HEAD(list);
	struct resource *res1, *res2, *root = NULL;
	struct resource_entry *tmp, *entry, *entry2;

	BUG_ON((type & (IORESOURCE_MEM | IORESOURCE_IO)) == 0);
	root = (type & IORESOURCE_MEM) ? &iomem_resource : &ioport_resource;

	list_splice_init(resources, &list);
	resource_list_for_each_entry_safe(entry, tmp, &list) {
		bool free = false;
		resource_size_t end;

		res1 = entry->res;
		if (!(res1->flags & type))
			goto next;

		/* Exclude non-addressable range or non-addressable portion */
		end = min(res1->end, root->end);
		if (end <= res1->start) {
			dev_info(dev, "host bridge window %pR (ignored, not CPU addressable)\n",
				 res1);
			free = true;
			goto next;
		} else if (res1->end != end) {
			dev_info(dev, "host bridge window %pR ([%#llx-%#llx] ignored, not CPU addressable)\n",
				 res1, (unsigned long long)end + 1,
				 (unsigned long long)res1->end);
			res1->end = end;
		}

		resource_list_for_each_entry(entry2, resources) {
			res2 = entry2->res;
			if (!(res2->flags & type))
				continue;

			/*
			 * I don't like throwing away windows because then
			 * our resources no longer match the ACPI _CRS, but
			 * the kernel resource tree doesn't allow overlaps.
			 */
			if (resource_union(res1, res2, res2)) {
				dev_info(dev, "host bridge window expanded to %pR; %pR ignored\n",
					 res2, res1);
				free = true;
				goto next;
			}
		}

next:
		resource_list_del(entry);
		if (free)
			resource_list_free_entry(entry);
		else
			resource_list_add_tail(entry, resources);
	}
}
/* …（同文件无关代码省略）… */
static void acpi_pci_root_remap_iospace(const struct fwnode_handle *fwnode,
			struct resource_entry *entry)
{
#ifdef PCI_IOBASE
	struct resource *res = entry->res;
	resource_size_t cpu_addr = res->start;
	resource_size_t pci_addr = cpu_addr - entry->offset;
	resource_size_t length = resource_size(res);
	unsigned long port;

	if (pci_register_io_range(fwnode, cpu_addr, length))
		goto err;

	port = pci_address_to_pio(cpu_addr);
	if (port == (unsigned long)-1)
		goto err;

	res->start = port;
	res->end = port + length - 1;
	entry->offset = port - pci_addr;

	if (pci_remap_iospace(res, cpu_addr) < 0)
		goto err;

	pr_info("Remapped I/O %pa to %pR\n", &cpu_addr, res);
	return;
err:
	res->flags |= IORESOURCE_DISABLED;
#endif
}
/* …（同文件无关代码省略）… */
int acpi_pci_probe_root_resources(struct acpi_pci_root_info *info)
{
	int ret;
	struct list_head *list = &info->resources;
	struct acpi_device *device = info->bridge;
	struct resource_entry *entry, *tmp;
	unsigned long flags;

	flags = IORESOURCE_IO | IORESOURCE_MEM | IORESOURCE_MEM_8AND16BIT;
	ret = acpi_dev_get_resources(device, list,
				     acpi_dev_filter_resource_type_cb,
				     (void *)flags);
	if (ret < 0)
		dev_warn(&device->dev,
			 "failed to parse _CRS method, error code %d\n", ret);
	else if (ret == 0)
		dev_dbg(&device->dev,
			"no IO and memory resources present in _CRS\n");
	else {
		resource_list_for_each_entry_safe(entry, tmp, list) {
			if (entry->res->flags & IORESOURCE_IO)
				acpi_pci_root_remap_iospace(&device->fwnode,
						entry);

			if (entry->res->flags & IORESOURCE_DISABLED)
				resource_list_destroy_entry(entry);
			else
				entry->res->name = info->name;
		}
		acpi_pci_root_validate_resources(&device->dev, list,
						 IORESOURCE_MEM);
		acpi_pci_root_validate_resources(&device->dev, list,
						 IORESOURCE_IO);
	}

	return ret;
}
/* …（同文件无关代码省略）… */
static void pci_acpi_root_add_resources(struct acpi_pci_root_info *info)
{
	struct resource_entry *entry, *tmp;
	struct resource *res, *conflict, *root = NULL;

	resource_list_for_each_entry_safe(entry, tmp, &info->resources) {
		res = entry->res;
		if (res->flags & IORESOURCE_MEM)
			root = &iomem_resource;
		else if (res->flags & IORESOURCE_IO)
			root = &ioport_resource;
		else
			continue;

		/*
		 * Some legacy x86 host bridge drivers use iomem_resource and
		 * ioport_resource as default resource pool, skip it.
		 */
		if (res == root)
			continue;

		conflict = insert_resource_conflict(root, res);
		if (conflict) {
			dev_info(&info->bridge->dev,
				 "ignoring host bridge window %pR (conflicts with %s %pR)\n",
				 res, conflict->name, conflict);
			resource_list_destroy_entry(entry);
		}
	}
}
/* …（同文件无关代码省略）… */
static void __acpi_pci_root_release_info(struct acpi_pci_root_info *info)
{
	struct resource *res;
	struct resource_entry *entry, *tmp;

	if (!info)
		return;

	resource_list_for_each_entry_safe(entry, tmp, &info->resources) {
		res = entry->res;
		if (res->parent &&
		    (res->flags & (IORESOURCE_MEM | IORESOURCE_IO)))
			release_resource(res);
		resource_list_destroy_entry(entry);
	}

	info->ops->release_info(info);
}
/* …（同文件无关代码省略）… */
static void acpi_pci_root_release_info(struct pci_host_bridge *bridge)
{
	struct resource *res;
	struct resource_entry *entry;

	resource_list_for_each_entry(entry, &bridge->windows) {
		res = entry->res;
		if (res->flags & IORESOURCE_IO)
			pci_unmap_iospace(res);
		if (res->parent &&
		    (res->flags & (IORESOURCE_MEM | IORESOURCE_IO)))
			release_resource(res);
	}
	__acpi_pci_root_release_info(bridge->release_data);
}

struct pci_bus *acpi_pci_root_create(struct acpi_pci_root *root,
				     struct acpi_pci_root_ops *ops,
				     struct acpi_pci_root_info *info,
				     void *sysdata)
{
	int ret, busnum = root->secondary.start;
	struct acpi_device *device = root->device;
	int node = acpi_get_node(device->handle);
	struct pci_bus *bus;
	struct pci_host_bridge *host_bridge;

	info->root = root;
	info->bridge = device;
	info->ops = ops;
	INIT_LIST_HEAD(&info->resources);
	snprintf(info->name, sizeof(info->name), "PCI Bus %04x:%02x",
		 root->segment, busnum);

	if (ops->init_info && ops->init_info(info))
		goto out_release_info;
	if (ops->prepare_resources)
		ret = ops->prepare_resources(info);
	else
		ret = acpi_pci_probe_root_resources(info);
	if (ret < 0)
		goto out_release_info;

	pci_acpi_root_add_resources(info);
	pci_add_resource(&info->resources, &root->secondary);
	bus = pci_create_root_bus(NULL, busnum, ops->pci_ops,
				  sysdata, &info->resources);
	if (!bus)
		goto out_release_info;

	host_bridge = to_pci_host_bridge(bus->bridge);
	if (!(root->osc_control_set & OSC_PCI_EXPRESS_NATIVE_HP_CONTROL))
		host_bridge->native_pcie_hotplug = 0;
	if (!(root->osc_control_set & OSC_PCI_SHPC_NATIVE_HP_CONTROL))
		host_bridge->native_shpc_hotplug = 0;
	if (!(root->osc_control_set & OSC_PCI_EXPRESS_AER_CONTROL))
		host_bridge->native_aer = 0;
	if (!(root->osc_control_set & OSC_PCI_EXPRESS_PME_CONTROL))
		host_bridge->native_pme = 0;
	if (!(root->osc_control_set & OSC_PCI_EXPRESS_LTR_CONTROL))
		host_bridge->native_ltr = 0;
	if (!(root->osc_control_set & OSC_PCI_EXPRESS_DPC_CONTROL))
		host_bridge->native_dpc = 0;

	if (!(root->osc_ext_control_set & OSC_CXL_ERROR_REPORTING_CONTROL))
		host_bridge->native_cxl_error = 0;

	acpi_dev_power_up_children_with_adr(device);

	pci_scan_child_bus(bus);
	pci_set_host_bridge_release(host_bridge, acpi_pci_root_release_info,
				    info);
	if (node != NUMA_NO_NODE)
		dev_printk(KERN_DEBUG, &bus->dev, "on NUMA node %d\n", node);
	return bus;

out_release_info:
	__acpi_pci_root_release_info(info);
	return NULL;
}
