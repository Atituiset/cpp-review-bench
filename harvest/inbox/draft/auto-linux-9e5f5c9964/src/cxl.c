// AUTO-DRAFT from torvalds/linux PR #b98adc197f7300e875fbcd4cf5dfa5ebe3d6dc08
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define NR_CXL_HOST_BRIDGES 2
#define NR_CXL_SINGLE_HOST 1
/* …（同文件无关代码省略）… */
#define NR_CXL_ROOT_PORTS 2
#define NR_CXL_SWITCH_PORTS 2
/* …（同文件无关代码省略）… */
#define NR_MULTI_ROOT (NR_CXL_HOST_BRIDGES * NR_CXL_ROOT_PORTS)
/* …（同文件无关代码省略）… */
#define NR_MEM_MULTI \
	(NR_CXL_HOST_BRIDGES * NR_CXL_ROOT_PORTS * NR_CXL_SWITCH_PORTS)
/* …（同文件无关代码省略）… */
static void mock_companion(struct acpi_device *adev, struct device *dev)
{
	device_initialize(&adev->dev);
	fwnode_init(&adev->fwnode, NULL);
	dev->fwnode = &adev->fwnode;
	adev->fwnode.dev = dev;
}
/* …（同文件无关代码省略）… */
static int cxl_mock_platform_device_add(struct platform_device *pdev,
					struct platform_device **ppdev)
{
	int rc;

	if (ppdev)
		*ppdev = pdev;
	rc = platform_device_add(pdev);
	if (rc) {
		platform_device_put(pdev);
		if (ppdev)
			*ppdev = NULL;
	}

	return rc;
}
/* …（同文件无关代码省略）… */
static __init int cxl_rch_topo_init(void)
{
	int rc, i;

	for (i = 0; i < ARRAY_SIZE(cxl_rch); i++) {
		int idx = NR_CXL_HOST_BRIDGES + NR_CXL_SINGLE_HOST + i;
		struct acpi_device *adev = &host_bridge[idx];
		struct platform_device *pdev;

		pdev = platform_device_alloc("cxl_host_bridge", idx);
		if (!pdev) {
			rc = -ENOMEM;
			goto err_bridge;
		}

		mock_companion(adev, &pdev->dev);
		rc = cxl_mock_platform_device_add(pdev, &cxl_rch[i]);
		if (rc)
			goto err_bridge;

		mock_pci_bus[idx].bridge = &pdev->dev;
		rc = sysfs_create_link(&pdev->dev.kobj, &pdev->dev.kobj,
				       "firmware_node");
		if (rc)
			goto err_bridge;
	}

	return 0;

err_bridge:
	for (i = ARRAY_SIZE(cxl_rch) - 1; i >= 0; i--) {
		struct platform_device *pdev = cxl_rch[i];

		if (!pdev)
			continue;
		sysfs_remove_link(&pdev->dev.kobj, "firmware_node");
		platform_device_unregister(cxl_rch[i]);
	}

	return rc;
}
/* …（同文件无关代码省略）… */
static __init int cxl_single_topo_init(void)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(cxl_hb_single); i++) {
		struct acpi_device *adev =
			&host_bridge[NR_CXL_HOST_BRIDGES + i];
		struct platform_device *pdev;

		pdev = platform_device_alloc("cxl_host_bridge",
					     NR_CXL_HOST_BRIDGES + i);
		if (!pdev) {
			rc = -ENOMEM;
			goto err_bridge;
		}

		mock_companion(adev, &pdev->dev);
		rc = cxl_mock_platform_device_add(pdev, &cxl_hb_single[i]);
		if (rc)
			goto err_bridge;

		mock_pci_bus[i + NR_CXL_HOST_BRIDGES].bridge = &pdev->dev;
		rc = sysfs_create_link(&pdev->dev.kobj, &pdev->dev.kobj,
				       "physical_node");
		if (rc)
			goto err_bridge;
	}

	for (i = 0; i < ARRAY_SIZE(cxl_root_single); i++) {
		struct platform_device *bridge =
			cxl_hb_single[i % ARRAY_SIZE(cxl_hb_single)];
		struct platform_device *pdev;

		pdev = platform_device_alloc("cxl_root_port",
					     NR_MULTI_ROOT + i);
		if (!pdev) {
			rc = -ENOMEM;
			goto err_port;
		}
		pdev->dev.parent = &bridge->dev;

		rc = cxl_mock_platform_device_add(pdev, &cxl_root_single[i]);
		if (rc)
			goto err_port;
	}

	for (i = 0; i < ARRAY_SIZE(cxl_swu_single); i++) {
		struct platform_device *root_port = cxl_root_single[i];
		struct platform_device *pdev;

		pdev = platform_device_alloc("cxl_switch_uport",
					     NR_MULTI_ROOT + i);
		if (!pdev) {
			rc = -ENOMEM;
			goto err_uport;
		}
		pdev->dev.parent = &root_port->dev;

		rc = cxl_mock_platform_device_add(pdev, &cxl_swu_single[i]);
		if (rc)
			goto err_uport;
	}

	for (i = 0; i < ARRAY_SIZE(cxl_swd_single); i++) {
		struct platform_device *uport =
			cxl_swu_single[i % ARRAY_SIZE(cxl_swu_single)];
		struct platform_device *pdev;

		pdev = platform_device_alloc("cxl_switch_dport",
					     i + NR_MEM_MULTI);
		if (!pdev) {
			rc = -ENOMEM;
			goto err_dport;
		}
		pdev->dev.parent = &uport->dev;

		rc = cxl_mock_platform_device_add(pdev, &cxl_swd_single[i]);
		if (rc)
			goto err_dport;
	}

	return 0;

err_dport:
	for (i = ARRAY_SIZE(cxl_swd_single) - 1; i >= 0; i--)
		platform_device_unregister(cxl_swd_single[i]);
err_uport:
	for (i = ARRAY_SIZE(cxl_swu_single) - 1; i >= 0; i--)
		platform_device_unregister(cxl_swu_single[i]);
err_port:
	for (i = ARRAY_SIZE(cxl_root_single) - 1; i >= 0; i--)
		platform_device_unregister(cxl_root_single[i]);
err_bridge:
	for (i = ARRAY_SIZE(cxl_hb_single) - 1; i >= 0; i--) {
		struct platform_device *pdev = cxl_hb_single[i];

		if (!pdev)
			continue;
		sysfs_remove_link(&pdev->dev.kobj, "physical_node");
		platform_device_unregister(cxl_hb_single[i]);
	}

	return rc;
}
/* …（同文件无关代码省略）… */
static void cxl_single_topo_exit(void)
{
	int i;

	for (i = ARRAY_SIZE(cxl_swd_single) - 1; i >= 0; i--)
		platform_device_unregister(cxl_swd_single[i]);
	for (i = ARRAY_SIZE(cxl_swu_single) - 1; i >= 0; i--)
		platform_device_unregister(cxl_swu_single[i]);
	for (i = ARRAY_SIZE(cxl_root_single) - 1; i >= 0; i--)
		platform_device_unregister(cxl_root_single[i]);
	for (i = ARRAY_SIZE(cxl_hb_single) - 1; i >= 0; i--) {
		struct platform_device *pdev = cxl_hb_single[i];

		if (!pdev)
			continue;
		sysfs_remove_link(&pdev->dev.kobj, "physical_node");
		platform_device_unregister(cxl_hb_single[i]);
	}
}
/* …（同文件无关代码省略）… */
static void host_bridges_remove(void)
{
	int i;

	for (i = ARRAY_SIZE(cxl_host_bridge) - 1; i >= 0; i--) {
		struct platform_device *pdev = cxl_host_bridge[i];

		if (!pdev)
			continue;

		sysfs_remove_link(&pdev->dev.kobj, "physical_node");
		platform_device_unregister(cxl_host_bridge[i]);
	}
}
/* …（同文件无关代码省略）… */
static int host_bridges_populate(void)
{
	int rc = 0;

	for (int i = 0; i < ARRAY_SIZE(cxl_host_bridge); i++) {
		struct acpi_device *adev = &host_bridge[i];
		struct platform_device *pdev;

		pdev = platform_device_alloc("cxl_host_bridge", i);
		if (!pdev) {
			rc = -ENOMEM;
			goto err_bridge;
		}

		mock_companion(adev, &pdev->dev);
		rc = cxl_mock_platform_device_add(pdev, &cxl_host_bridge[i]);
		if (rc)
			goto err_bridge;

		mock_pci_bus[i].bridge = &pdev->dev;
		rc = sysfs_create_link(&pdev->dev.kobj, &pdev->dev.kobj,
				       "physical_node");
		if (rc)
			goto err_bridge;
	}

	return 0;

err_bridge:
	host_bridges_remove();
	return rc;
}
/* …（同文件无关代码省略）… */
static void cxl_rootports_remove(void)
{
	for (int i = ARRAY_SIZE(cxl_root_port) - 1; i >= 0; i--) {
		struct platform_device *pdev = cxl_root_port[i];

		if (!pdev)
			continue;

		platform_device_unregister(pdev);
	}
}
/* …（同文件无关代码省略）… */
static int cxl_rootports_populate(void)
{
	int rc = 0;

	for (int i = 0; i < ARRAY_SIZE(cxl_root_port); i++) {
		struct platform_device *bridge =
			cxl_host_bridge[i % ARRAY_SIZE(cxl_host_bridge)];
		struct platform_device *pdev;

		pdev = platform_device_alloc("cxl_root_port", i);
		if (!pdev) {
			rc = -ENOMEM;
			goto err_port;
		}

		pdev->dev.parent = &bridge->dev;

		rc = cxl_mock_platform_device_add(pdev, &cxl_root_port[i]);
		if (rc)
			goto err_port;
	}

	return 0;

err_port:
	cxl_rootports_remove();
	return rc;
}
/* …（同文件无关代码省略）… */
static void cxl_usps_remove(void)
{
	for (int i = ARRAY_SIZE(cxl_switch_uport) - 1; i >= 0; i--) {
		struct platform_device *pdev = cxl_switch_uport[i];

		if (!pdev)
			continue;

		platform_device_unregister(cxl_switch_uport[i]);
	}
}
/* …（同文件无关代码省略）… */
static int cxl_usps_populate(void)
{
	int rc = 0;

	for (int i = 0; i < ARRAY_SIZE(cxl_switch_uport); i++) {
		struct platform_device *root_port = cxl_root_port[i];
		struct platform_device *pdev;

		pdev = platform_device_alloc("cxl_switch_uport", i);
		if (!pdev) {
			rc = -ENOMEM;
			goto err_uport;
		}

		pdev->dev.parent = &root_port->dev;

		rc = cxl_mock_platform_device_add(pdev, &cxl_switch_uport[i]);
		if (rc)
			goto err_uport;
	}

	return 0;

err_uport:
	cxl_usps_remove();
	return rc;
}
/* …（同文件无关代码省略）… */
static void cxl_dsps_remove(void)
{
	for (int i = ARRAY_SIZE(cxl_switch_dport) - 1; i >= 0; i--) {
		struct platform_device *pdev = cxl_switch_dport[i];

		if (!pdev)
			continue;

		platform_device_unregister(cxl_switch_dport[i]);
	}
}
/* …（同文件无关代码省略）… */
static int cxl_dsps_populate(void)
{
	int rc = 0;

	for (int i = 0; i < ARRAY_SIZE(cxl_switch_dport); i++) {
		struct platform_device *uport =
			cxl_switch_uport[i % ARRAY_SIZE(cxl_switch_uport)];
		struct platform_device *pdev;

		pdev = platform_device_alloc("cxl_switch_dport", i);
		if (!pdev) {
			rc = -ENOMEM;
			goto err_dport;
		}
		pdev->dev.parent = &uport->dev;

		rc = cxl_mock_platform_device_add(pdev, &cxl_switch_dport[i]);
		if (rc)
			goto err_dport;
	}

	return 0;

err_dport:
	cxl_dsps_remove();
	return rc;
}
/* …（同文件无关代码省略）… */
static void cxl_switches_remove(void)
{
	cxl_dsps_remove();
	cxl_usps_remove();
}
/* …（同文件无关代码省略）… */
static int cxl_switches_populate(void)
{
	int rc;

	BUILD_BUG_ON(ARRAY_SIZE(cxl_switch_uport) != ARRAY_SIZE(cxl_root_port));
	rc = cxl_usps_populate();
	if (rc)
		return rc;

	rc = cxl_dsps_populate();
	if (rc) {
		cxl_usps_remove();
		return rc;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static int cxl_type3_topo_init(void)
{
	int rc;

	rc = host_bridges_populate();
	if (rc)
		return rc;

	rc = cxl_rootports_populate();
	if (rc)
		goto err_host_bridges;

	rc = cxl_switches_populate();
	if (rc)
		goto err_root_ports;

	rc = cxl_single_topo_init();
	if (rc)
		goto err_switches;

	rc = cxl_rch_topo_init();
	if (rc)
		goto err_single;

	return 0;

err_single:
	cxl_single_topo_exit();
err_switches:
	cxl_switches_remove();
err_root_ports:
	cxl_rootports_remove();
err_host_bridges:
	host_bridges_remove();
	return rc;
}
/* …（同文件无关代码省略）… */
	return cxl_type3_topo_init();
}

static __init int cxl_test_init(void)
{
	struct range mappable;
/* …（同文件无关代码省略）… */
	gen_pool_destroy(cxl_mock_pool);
err_gen_pool_create:
	unregister_cxl_mock_ops(&cxl_mock_ops);
	return rc;
}

static void free_decoder_registry(void)
{
	unsigned long index;
	void *entry;

	xa_for_each(&decoder_registry, index, entry) {
		xa_erase(&decoder_registry, index);
		kfree(entry);
	}
}

static __exit void cxl_test_exit(void)
{
	hmem_test_exit();
