// AUTO-DRAFT from torvalds/linux PR #786262be6048deab760f68c8acc2c85607165894
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define EDAC_DEVICE		"Altera"
/* …（同文件无关代码省略）… */
static void altr_create_edacdev_dbgfs(struct edac_device_ctl_info *edac_dci,
				      const struct edac_device_prv_data *priv)
{
	struct altr_edac_device_dev *drvdata = edac_dci->pvt_info;

	if (!IS_ENABLED(CONFIG_EDAC_DEBUG))
		return;

	drvdata->debugfs_dir = edac_debugfs_create_dir(drvdata->edac_dev_name);
	if (!drvdata->debugfs_dir)
		return;

	if (!edac_debugfs_create_file("altr_trigger", S_IWUSR,
				      drvdata->debugfs_dir, edac_dci,
				      priv->inject_fops))
		debugfs_remove_recursive(drvdata->debugfs_dir);
}
/* …（同文件无关代码省略）… */
static int __maybe_unused
altr_check_ecc_deps(struct altr_edac_device_dev *device)
{
	void __iomem  *base = device->base;
	const struct edac_device_prv_data *prv = device->data;

	if (readl(base + prv->ecc_en_ofst) & prv->ecc_enable_mask)
		return 0;

	edac_printk(KERN_ERR, EDAC_DEVICE,
		    "%s: No ECC present or ECC disabled.\n",
		    device->edac_dev_name);
	return -ENODEV;
}
/* …（同文件无关代码省略）… */
	int edac_idx, rc;
	struct device_node *np;
	const struct edac_device_prv_data *prv = &a10_sdmmceccb_data;

	rc = altr_check_ecc_deps(device);
	if (rc)
/* …（同文件无关代码省略）… */

	/*
	 * Update the PortB IRQs - A10 has 4, S10 has 2, Index accordingly
	 *
	 * FIXME: Instead of ifdefs with different architectures the driver
	 *        should properly use compatibles.
	 */
#ifdef CONFIG_64BIT
	altdev->sb_irq = irq_of_parse_and_map(np, 1);
#else
	altdev->sb_irq = irq_of_parse_and_map(np, 2);
#endif
	if (!altdev->sb_irq) {
		edac_printk(KERN_ERR, EDAC_DEVICE, "Error PortB SBIRQ alloc\n");
		rc = -ENODEV;
/* …（同文件无关代码省略）… */
		goto err_release_group_1;
	}

#ifdef CONFIG_64BIT
	/* Use IRQ to determine SError origin instead of assigning IRQ */
	rc = of_property_read_u32_index(np, "interrupts", 1, &altdev->db_irq);
	if (rc) {
		edac_printk(KERN_ERR, EDAC_DEVICE,
			    "Error PortB DBIRQ alloc\n");
		goto err_release_group_1;
	}
#else
	altdev->db_irq = irq_of_parse_and_map(np, 3);
	if (!altdev->db_irq) {
		edac_printk(KERN_ERR, EDAC_DEVICE, "Error PortB DBIRQ alloc\n");
		rc = -ENODEV;
		goto err_release_group_1;
	}
	rc = devm_request_irq(&altdev->ddev, altdev->db_irq,
			      prv->ecc_irq_handler, IRQF_TRIGGER_HIGH,
			      ecc_name, altdev);
	if (rc) {
		edac_printk(KERN_ERR, EDAC_DEVICE, "PortB DBERR IRQ error\n");
		goto err_release_group_1;
	}
#endif

	rc = edac_device_add_device(dci);
	if (rc) {
/* …（同文件无关代码省略）… */
static void altr_edac_a10_irq_handler(struct irq_desc *desc)
{
	int dberr, bit, sm_offset, irq_status;
	struct altr_arria10_edac *edac = irq_desc_get_handler_data(desc);
	struct irq_chip *chip = irq_desc_get_chip(desc);
	int irq = irq_desc_get_irq(desc);
	unsigned long bits;

	dberr = (irq == edac->db_irq) ? 1 : 0;
	sm_offset = dberr ? A10_SYSMGR_ECC_INTSTAT_DERR_OFST :
			    A10_SYSMGR_ECC_INTSTAT_SERR_OFST;

	chained_irq_enter(chip, desc);

	regmap_read(edac->ecc_mgr_map, sm_offset, &irq_status);

	bits = irq_status;
	for_each_set_bit(bit, &bits, 32)
		generic_handle_domain_irq(edac->domain, dberr * 32 + bit);

	chained_irq_exit(chip, desc);
}
/* …（同文件无关代码省略）… */
static int validate_parent_available(struct device_node *np)
{
	struct device_node *parent;
	int ret = 0;

	/* SDRAM must be present for Linux (implied parent) */
	if (of_device_is_compatible(np, "altr,sdram-edac-s10"))
		return 0;

	/* Ensure parent device is enabled if parent node exists */
	parent = of_parse_phandle(np, "altr,ecc-parent", 0);
	if (parent && !of_device_is_available(parent))
		ret = -ENODEV;

	of_node_put(parent);
	return ret;
}
/* …（同文件无关代码省略）… */
static int get_s10_sdram_edac_resource(struct device_node *np,
				       struct resource *res)
{
	struct device_node *parent;
	int ret;

	parent = of_parse_phandle(np, "altr,sdr-syscon", 0);
	if (!parent)
		return -ENODEV;

	ret = of_address_to_resource(parent, 0, res);
	of_node_put(parent);

	return ret;
}
/* …（同文件无关代码省略）… */
static int altr_edac_a10_device_add(struct altr_arria10_edac *edac,
				    struct device_node *np)
{
	struct edac_device_ctl_info *dci;
	struct altr_edac_device_dev *altdev;
	char *ecc_name = (char *)np->name;
	struct resource res;
	int edac_idx;
	int rc = 0;
	const struct edac_device_prv_data *prv;
	/* Get matching node and check for valid result */
	const struct of_device_id *pdev_id =
		of_match_node(altr_edac_a10_device_of_match, np);
	if (IS_ERR_OR_NULL(pdev_id))
		return -ENODEV;

	/* Get driver specific data for this EDAC device */
	prv = pdev_id->data;
	if (IS_ERR_OR_NULL(prv))
		return -ENODEV;

	if (validate_parent_available(np))
		return -ENODEV;

	if (!devres_open_group(edac->dev, altr_edac_a10_device_add, GFP_KERNEL))
		return -ENOMEM;

	if (of_device_is_compatible(np, "altr,sdram-edac-s10"))
		rc = get_s10_sdram_edac_resource(np, &res);
	else
		rc = of_address_to_resource(np, 0, &res);

	if (rc < 0) {
		edac_printk(KERN_ERR, EDAC_DEVICE,
			    "%s: no resource address\n", ecc_name);
		goto err_release_group;
	}

	edac_idx = edac_device_alloc_index();
	dci = edac_device_alloc_ctl_info(sizeof(*altdev), ecc_name,
					 1, ecc_name, 1, 0, edac_idx);

	if (!dci) {
		edac_printk(KERN_ERR, EDAC_DEVICE,
			    "%s: Unable to allocate EDAC device\n", ecc_name);
		rc = -ENOMEM;
		goto err_release_group;
	}

	altdev = dci->pvt_info;
	dci->dev = edac->dev;
	altdev->edac_dev_name = ecc_name;
	altdev->edac_idx = edac_idx;
	altdev->edac = edac;
	altdev->edac_dev = dci;
	altdev->data = prv;
	altdev->ddev = *edac->dev;
	dci->dev = &altdev->ddev;
	dci->ctl_name = "Altera ECC Manager";
	dci->mod_name = ecc_name;
	dci->dev_name = ecc_name;

	altdev->base = devm_ioremap_resource(edac->dev, &res);
	if (IS_ERR(altdev->base)) {
		rc = PTR_ERR(altdev->base);
		goto err_release_group1;
	}

	/* Check specific dependencies for the module */
	if (altdev->data->setup) {
		rc = altdev->data->setup(altdev);
		if (rc)
			goto err_release_group1;
	}

	altdev->sb_irq = irq_of_parse_and_map(np, 0);
