// AUTO-DRAFT from torvalds/linux PR #17bb0e2f6c8043f70a693a5f3797471cbed354c8
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR

#define CDNS_PLAT_CPU_TO_BUS_ADDR	0x0FFFFFFF

/**
 * struct cdns_plat_pcie - private data for this PCIe platform driver
 * @pcie: Cadence PCIe controller
 */
struct cdns_plat_pcie {
	struct cdns_pcie        *pcie;
};

static const struct of_device_id cdns_plat_pcie_of_match[];

static u64 cdns_plat_cpu_addr_fixup(struct cdns_pcie *pcie, u64 cpu_addr)
/* …（同文件无关代码省略）… */
static int cdns_plat_pcie_probe(struct platform_device *pdev)
{
	const struct cdns_plat_pcie_of_data *data;
	struct cdns_plat_pcie *cdns_plat_pcie;
	struct device *dev = &pdev->dev;
	struct pci_host_bridge *bridge;
	struct cdns_pcie_ep *ep;
	struct cdns_pcie_rc *rc;
	int phy_count;
	bool is_rc;
	int ret;

	data = of_device_get_match_data(dev);
	if (!data)
		return -EINVAL;

	is_rc = data->is_rc;

	pr_debug(" Started %s with is_rc: %d\n", __func__, is_rc);
	cdns_plat_pcie = devm_kzalloc(dev, sizeof(*cdns_plat_pcie), GFP_KERNEL);
	if (!cdns_plat_pcie)
		return -ENOMEM;

	platform_set_drvdata(pdev, cdns_plat_pcie);
	if (is_rc) {
		if (!IS_ENABLED(CONFIG_PCIE_CADENCE_PLAT_HOST))
			return -ENODEV;

		bridge = devm_pci_alloc_host_bridge(dev, sizeof(*rc));
		if (!bridge)
			return -ENOMEM;

		rc = pci_host_bridge_priv(bridge);
		rc->pcie.dev = dev;
		rc->pcie.ops = &cdns_plat_ops;
		cdns_plat_pcie->pcie = &rc->pcie;

		ret = cdns_pcie_init_phy(dev, cdns_plat_pcie->pcie);
		if (ret) {
			dev_err(dev, "failed to init phy\n");
			return ret;
		}
		pm_runtime_enable(dev);
		ret = pm_runtime_get_sync(dev);
		if (ret < 0) {
			dev_err(dev, "pm_runtime_get_sync() failed\n");
			goto err_get_sync;
		}

		ret = cdns_pcie_host_setup(rc);
		if (ret)
			goto err_init;
	} else {
		if (!IS_ENABLED(CONFIG_PCIE_CADENCE_PLAT_EP))
			return -ENODEV;

		ep = devm_kzalloc(dev, sizeof(*ep), GFP_KERNEL);
		if (!ep)
			return -ENOMEM;

		ep->pcie.dev = dev;
		ep->pcie.ops = &cdns_plat_ops;
		cdns_plat_pcie->pcie = &ep->pcie;

		ret = cdns_pcie_init_phy(dev, cdns_plat_pcie->pcie);
		if (ret) {
			dev_err(dev, "failed to init phy\n");
			return ret;
		}

		pm_runtime_enable(dev);
		ret = pm_runtime_get_sync(dev);
		if (ret < 0) {
			dev_err(dev, "pm_runtime_get_sync() failed\n");
			goto err_get_sync;
		}

		ret = cdns_pcie_ep_setup(ep);
		if (ret)
			goto err_init;
	}

	return 0;

 err_init:
 err_get_sync:
	pm_runtime_put_sync(dev);
	pm_runtime_disable(dev);
	cdns_pcie_disable_phy(cdns_plat_pcie->pcie);
	phy_count = cdns_plat_pcie->pcie->phy_count;
	while (phy_count--)
		device_link_del(cdns_plat_pcie->pcie->link[phy_count]);

	return 0;
}

static void cdns_plat_pcie_shutdown(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cdns_pcie *pcie = dev_get_drvdata(dev);
	int ret;

	ret = pm_runtime_put_sync(dev);
	if (ret < 0)
		dev_dbg(dev, "pm_runtime_put_sync failed\n");

	pm_runtime_disable(dev);

	cdns_pcie_disable_phy(pcie);
}
