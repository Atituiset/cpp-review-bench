// AUTO-DRAFT from torvalds/linux PR #6addb4f385570ebc11c4eb499a4f1c149f313e84
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define EDAC_MOD_STR		"altera_edac"
#define EDAC_DEVICE		"Altera"
/* …（同文件无关代码省略）… */
static irqreturn_t altr_sdram_mc_err_handler(int irq, void *dev_id)
{
	struct mem_ctl_info *mci = dev_id;
	struct altr_sdram_mc_data *drvdata = mci->pvt_info;
	const struct altr_sdram_prv_data *priv = drvdata->data;
	u32 status, err_count = 1, err_addr;

	regmap_read(drvdata->mc_vbase, priv->ecc_stat_offset, &status);

	if (status & priv->ecc_stat_ue_mask) {
		regmap_read(drvdata->mc_vbase, priv->ecc_daddr_offset,
			    &err_addr);
		if (priv->ecc_uecnt_offset)
			regmap_read(drvdata->mc_vbase, priv->ecc_uecnt_offset,
				    &err_count);
		panic("\nEDAC: [%d Uncorrectable errors @ 0x%08X]\n",
		      err_count, err_addr);
	}
	if (status & priv->ecc_stat_ce_mask) {
		regmap_read(drvdata->mc_vbase, priv->ecc_saddr_offset,
			    &err_addr);
		if (priv->ecc_cecnt_offset)
			regmap_read(drvdata->mc_vbase,  priv->ecc_cecnt_offset,
				    &err_count);
		edac_mc_handle_error(HW_EVENT_ERR_CORRECTED, mci, err_count,
				     err_addr >> PAGE_SHIFT,
				     err_addr & ~PAGE_MASK, 0,
				     0, 0, -1, mci->ctl_name, "");
		/* Clear IRQ to resume */
		regmap_write(drvdata->mc_vbase,	priv->ecc_irq_clr_offset,
			     priv->ecc_irq_clr_mask);

		return IRQ_HANDLED;
	}
	return IRQ_NONE;
}
/* …（同文件无关代码省略）… */
static void altr_sdr_mc_create_debugfs_nodes(struct mem_ctl_info *mci)
{
	if (!IS_ENABLED(CONFIG_EDAC_DEBUG))
		return;

	if (!mci->debugfs)
		return;

	edac_debugfs_create_file("altr_trigger", S_IWUSR, mci->debugfs, mci,
				 &altr_sdr_mc_debug_inject_fops);
}
/* …（同文件无关代码省略）… */
static unsigned long get_total_mem(void)
{
	struct device_node *np = NULL;
	struct resource res;
	int ret;
	unsigned long total_mem = 0;

	for_each_node_by_type(np, "memory") {
		ret = of_address_to_resource(np, 0, &res);
		if (ret)
			continue;

		total_mem += resource_size(&res);
	}
	edac_dbg(0, "total_mem 0x%lx\n", total_mem);
	return total_mem;
}
/* …（同文件无关代码省略）… */
static int a10_init(struct regmap *mc_vbase)
{
	if (regmap_update_bits(mc_vbase, A10_INTMODE_OFST,
			       A10_INTMODE_SB_INT, A10_INTMODE_SB_INT)) {
		edac_printk(KERN_ERR, EDAC_MC,
			    "Error setting SB IRQ mode\n");
		return -ENODEV;
	}

	if (regmap_write(mc_vbase, A10_SERRCNTREG_OFST, 1)) {
		edac_printk(KERN_ERR, EDAC_MC,
			    "Error setting trigger count\n");
		return -ENODEV;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static int a10_unmask_irq(struct platform_device *pdev, u32 mask)
{
	void __iomem  *sm_base;
	int  ret = 0;

	if (!request_mem_region(A10_SYMAN_INTMASK_CLR, sizeof(u32),
				dev_name(&pdev->dev))) {
		edac_printk(KERN_ERR, EDAC_MC,
			    "Unable to request mem region\n");
		return -EBUSY;
	}

	sm_base = ioremap(A10_SYMAN_INTMASK_CLR, sizeof(u32));
	if (!sm_base) {
		edac_printk(KERN_ERR, EDAC_MC,
			    "Unable to ioremap device\n");

		ret = -ENOMEM;
		goto release;
	}

	iowrite32(mask, sm_base);

	iounmap(sm_base);

release:
	release_mem_region(A10_SYMAN_INTMASK_CLR, sizeof(u32));

	return ret;
}
/* …（同文件无关代码省略）… */
static int altr_sdram_probe(struct platform_device *pdev)
{
	struct edac_mc_layer layers[2];
	struct mem_ctl_info *mci;
	struct altr_sdram_mc_data *drvdata;
	const struct altr_sdram_prv_data *priv;
	struct regmap *mc_vbase;
	struct dimm_info *dimm;
	u32 read_reg;
	int irq, irq2, res = 0;
	unsigned long mem_size, irqflags = 0;

	/* Grab the register range from the sdr controller in device tree */
	mc_vbase = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
						   "altr,sdr-syscon");
	if (IS_ERR(mc_vbase)) {
		edac_printk(KERN_ERR, EDAC_MC,
			    "regmap for altr,sdr-syscon lookup failed.\n");
		return -ENODEV;
	}

	/* Check specific dependencies for the module */
	priv = device_get_match_data(&pdev->dev);

	/* Validate the SDRAM controller has ECC enabled */
	if (regmap_read(mc_vbase, priv->ecc_ctrl_offset, &read_reg) ||
	    ((read_reg & priv->ecc_ctl_en_mask) != priv->ecc_ctl_en_mask)) {
		edac_printk(KERN_ERR, EDAC_MC,
			    "No ECC/ECC disabled [0x%08X]\n", read_reg);
		return -ENODEV;
	}

	/* Grab memory size from device tree. */
	mem_size = get_total_mem();
	if (!mem_size) {
		edac_printk(KERN_ERR, EDAC_MC, "Unable to calculate memory size\n");
		return -ENODEV;
	}

	/* Ensure the SDRAM Interrupt is disabled */
	if (regmap_update_bits(mc_vbase, priv->ecc_irq_en_offset,
			       priv->ecc_irq_en_mask, 0)) {
		edac_printk(KERN_ERR, EDAC_MC,
			    "Error disabling SDRAM ECC IRQ\n");
		return -ENODEV;
	}

	/* Toggle to clear the SDRAM Error count */
	if (regmap_update_bits(mc_vbase, priv->ecc_cnt_rst_offset,
			       priv->ecc_cnt_rst_mask,
			       priv->ecc_cnt_rst_mask)) {
		edac_printk(KERN_ERR, EDAC_MC,
			    "Error clearing SDRAM ECC count\n");
		return -ENODEV;
	}

	if (regmap_update_bits(mc_vbase, priv->ecc_cnt_rst_offset,
			       priv->ecc_cnt_rst_mask, 0)) {
		edac_printk(KERN_ERR, EDAC_MC,
			    "Error clearing SDRAM ECC count\n");
		return -ENODEV;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		edac_printk(KERN_ERR, EDAC_MC,
			    "No irq %d in DT\n", irq);
		return irq;
	}

	/* Arria10 has a 2nd IRQ */
	irq2 = platform_get_irq(pdev, 1);

	layers[0].type = EDAC_MC_LAYER_CHIP_SELECT;
	layers[0].size = 1;
	layers[0].is_virt_csrow = true;
	layers[1].type = EDAC_MC_LAYER_CHANNEL;
	layers[1].size = 1;
	layers[1].is_virt_csrow = false;
	mci = edac_mc_alloc(0, ARRAY_SIZE(layers), layers,
			    sizeof(struct altr_sdram_mc_data));
	if (!mci)
		return -ENOMEM;

	mci->pdev = &pdev->dev;
	drvdata = mci->pvt_info;
	drvdata->mc_vbase = mc_vbase;
	drvdata->data = priv;
	platform_set_drvdata(pdev, mci);

	if (!devres_open_group(&pdev->dev, NULL, GFP_KERNEL)) {
		edac_printk(KERN_ERR, EDAC_MC,
			    "Unable to get managed device resource\n");
		res = -ENOMEM;
		goto free;
	}

	mci->mtype_cap = MEM_FLAG_DDR3;
	mci->edac_ctl_cap = EDAC_FLAG_NONE | EDAC_FLAG_SECDED;
	mci->edac_cap = EDAC_FLAG_SECDED;
	mci->mod_name = EDAC_MOD_STR;
	mci->ctl_name = dev_name(&pdev->dev);
	mci->scrub_mode = SCRUB_SW_SRC;
	mci->dev_name = dev_name(&pdev->dev);

	dimm = *mci->dimms;
	dimm->nr_pages = ((mem_size - 1) >> PAGE_SHIFT) + 1;
	dimm->grain = 8;
	dimm->dtype = DEV_X8;
	dimm->mtype = MEM_DDR3;
	dimm->edac_mode = EDAC_SECDED;

	res = edac_mc_add_mc(mci);
	if (res < 0)
		goto err;

	/* Only the Arria10 has separate IRQs */
	if (of_machine_is_compatible("altr,socfpga-arria10")) {
		/* Arria10 specific initialization */
		res = a10_init(mc_vbase);
		if (res < 0)
			goto err2;

		res = devm_request_irq(&pdev->dev, irq2,
				       altr_sdram_mc_err_handler,
				       IRQF_SHARED, dev_name(&pdev->dev), mci);
		if (res < 0) {
			edac_mc_printk(mci, KERN_ERR,
				       "Unable to request irq %d\n", irq2);
			res = -ENODEV;
			goto err2;
		}

		res = a10_unmask_irq(pdev, A10_DDR0_IRQ_MASK);
		if (res < 0)
			goto err2;

		irqflags = IRQF_SHARED;
	}

	res = devm_request_irq(&pdev->dev, irq, altr_sdram_mc_err_handler,
			       irqflags, dev_name(&pdev->dev), mci);
	if (res < 0) {
		edac_mc_printk(mci, KERN_ERR,
			       "Unable to request irq %d\n", irq);
		res = -ENODEV;
		goto err2;
	}

	/* Infrastructure ready - enable the IRQ */
	if (regmap_update_bits(drvdata->mc_vbase, priv->ecc_irq_en_offset,
			       priv->ecc_irq_en_mask, priv->ecc_irq_en_mask)) {
		edac_mc_printk(mci, KERN_ERR,
			       "Error enabling SDRAM ECC IRQ\n");
		res = -ENODEV;
		goto err2;
	}

	altr_sdr_mc_create_debugfs_nodes(mci);

	devres_close_group(&pdev->dev, NULL);

	return 0;

err2:
	edac_mc_del_mc(&pdev->dev);
err:
	devres_release_group(&pdev->dev, NULL);
free:
	edac_mc_free(mci);
	edac_printk(KERN_ERR, EDAC_MC,
		    "EDAC Probe Failed; Error %d\n", res);

	return res;
}

static void altr_sdram_remove(struct platform_device *pdev)
{
	struct mem_ctl_info *mci = platform_get_drvdata(pdev);

	edac_mc_del_mc(&pdev->dev);
	edac_mc_free(mci);
	platform_set_drvdata(pdev, NULL);
}

/*
 * If you want to suspend, need to disable EDAC by removing it
 * from the device tree or defconfig.
/* …（同文件无关代码省略）… */

static struct platform_driver altr_sdram_edac_driver = {
	.probe = altr_sdram_probe,
	.remove = altr_sdram_remove,
	.driver = {
		.name = "altr_sdram_edac",
#ifdef CONFIG_PM
		.pm = &altr_sdram_pm_ops,
#endif
		.of_match_table = altr_sdram_ctrl_of_match,
	},
};

/* …（同文件无关代码省略）… */
	.driver = {
		.name = "socfpga_ecc_manager",
		.of_match_table = altr_edac_of_match,
	},
};
module_platform_driver(altr_edac_driver);
/* …（同文件无关代码省略）… */
static irqreturn_t altr_edac_device_handler(int irq, void *dev_id)
{
	irqreturn_t ret_value = IRQ_NONE;
	struct edac_device_ctl_info *dci = dev_id;
	struct altr_edac_device_dev *drvdata = dci->pvt_info;
	const struct edac_device_prv_data *priv = drvdata->data;

	if (irq == drvdata->sb_irq) {
		if (priv->ce_clear_mask)
			writel(priv->ce_clear_mask, drvdata->base);
		edac_device_handle_ce(dci, 0, 0, drvdata->edac_dev_name);
		ret_value = IRQ_HANDLED;
	} else if (irq == drvdata->db_irq) {
		if (priv->ue_clear_mask)
			writel(priv->ue_clear_mask, drvdata->base);
		edac_device_handle_ue(dci, 0, 0, drvdata->edac_dev_name);
		panic("\nEDAC:ECC_DEVICE[Uncorrectable errors]\n");
		ret_value = IRQ_HANDLED;
	} else {
		WARN_ON(1);
	}

	return ret_value;
}
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
static int altr_edac_device_probe(struct platform_device *pdev)
{
	struct edac_device_ctl_info *dci;
	struct altr_edac_device_dev *drvdata;
	struct resource *r;
	int res = 0;
	struct device_node *np = pdev->dev.of_node;
	char *ecc_name = (char *)np->name;
	static int dev_instance;

	if (!devres_open_group(&pdev->dev, NULL, GFP_KERNEL)) {
		edac_printk(KERN_ERR, EDAC_DEVICE,
			    "Unable to open devm\n");
		return -ENOMEM;
	}

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!r) {
		edac_printk(KERN_ERR, EDAC_DEVICE,
			    "Unable to get mem resource\n");
		res = -ENODEV;
		goto fail;
	}

	if (!devm_request_mem_region(&pdev->dev, r->start, resource_size(r),
				     dev_name(&pdev->dev))) {
		edac_printk(KERN_ERR, EDAC_DEVICE,
			    "%s:Error requesting mem region\n", ecc_name);
		res = -EBUSY;
		goto fail;
	}

	dci = edac_device_alloc_ctl_info(sizeof(*drvdata), ecc_name,
					 1, ecc_name, 1, 0, dev_instance++);

	if (!dci) {
		edac_printk(KERN_ERR, EDAC_DEVICE,
			    "%s: Unable to allocate EDAC device\n", ecc_name);
		res = -ENOMEM;
		goto fail;
	}

	drvdata = dci->pvt_info;
	dci->dev = &pdev->dev;
	platform_set_drvdata(pdev, dci);
	drvdata->edac_dev_name = ecc_name;

	drvdata->base = devm_ioremap(&pdev->dev, r->start, resource_size(r));
	if (!drvdata->base) {
		res = -ENOMEM;
		goto fail1;
	}

	/* Get driver specific data for this EDAC device */
	drvdata->data = of_match_node(altr_edac_device_of_match, np)->data;

	/* Check specific dependencies for the module */
	if (drvdata->data->setup) {
		res = drvdata->data->setup(drvdata);
		if (res)
			goto fail1;
	}

	drvdata->sb_irq = platform_get_irq(pdev, 0);
	res = devm_request_irq(&pdev->dev, drvdata->sb_irq,
			       altr_edac_device_handler,
			       0, dev_name(&pdev->dev), dci);
	if (res)
		goto fail1;

	drvdata->db_irq = platform_get_irq(pdev, 1);
	res = devm_request_irq(&pdev->dev, drvdata->db_irq,
			       altr_edac_device_handler,
			       0, dev_name(&pdev->dev), dci);
	if (res)
		goto fail1;

	dci->mod_name = "Altera ECC Manager";
	dci->dev_name = drvdata->edac_dev_name;

	res = edac_device_add_device(dci);
	if (res)
		goto fail1;

	altr_create_edacdev_dbgfs(dci, drvdata->data);

	devres_close_group(&pdev->dev, NULL);

	return 0;

fail1:
	edac_device_free_ctl_info(dci);
fail:
	devres_release_group(&pdev->dev, NULL);
	edac_printk(KERN_ERR, EDAC_DEVICE,
		    "%s:Error setting up EDAC device: %d\n", ecc_name, res);

	return res;
}

static void altr_edac_device_remove(struct platform_device *pdev)
{
	struct edac_device_ctl_info *dci = platform_get_drvdata(pdev);
	struct altr_edac_device_dev *drvdata = dci->pvt_info;

	debugfs_remove_recursive(drvdata->debugfs_dir);
	edac_device_del_device(&pdev->dev);
	edac_device_free_ctl_info(dci);
}

static struct platform_driver altr_edac_device_driver = {
	.probe =  altr_edac_device_probe,
	.remove = altr_edac_device_remove,
	.driver = {
		.name = "altr_edac_device",
		.of_match_table = altr_edac_device_of_match,
	},
};
module_platform_driver(altr_edac_device_driver);
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
	return ret;
}

static __init int __maybe_unused
altr_init_a10_ecc_block(struct device_node *np, u32 irq_mask,
			u32 ecc_ctrl_en_mask, bool dual_port)
{
/* …（同文件无关代码省略）… */

static int validate_parent_available(struct device_node *np);
static const struct of_device_id altr_edac_a10_device_of_match[];
static int __init __maybe_unused altr_init_a10_ecc_device_type(char *compat)
{
	int irq;
	struct device_node *child, *np;
/* …（同文件无关代码省略）… */

#ifdef CONFIG_EDAC_ALTERA_ETHERNET

static int __init socfpga_init_ethernet_ecc(struct altr_edac_device_dev *dev)
{
	int ret;

/* …（同文件无关代码省略）… */

#ifdef CONFIG_EDAC_ALTERA_NAND

static int __init socfpga_init_nand_ecc(struct altr_edac_device_dev *device)
{
	int ret;

/* …（同文件无关代码省略）… */

#ifdef CONFIG_EDAC_ALTERA_DMA

static int __init socfpga_init_dma_ecc(struct altr_edac_device_dev *device)
{
	int ret;

/* …（同文件无关代码省略）… */

#ifdef CONFIG_EDAC_ALTERA_USB

static int __init socfpga_init_usb_ecc(struct altr_edac_device_dev *device)
{
	int ret;

/* …（同文件无关代码省略）… */

#ifdef CONFIG_EDAC_ALTERA_QSPI

static int __init socfpga_init_qspi_ecc(struct altr_edac_device_dev *device)
{
	int ret;

/* …（同文件无关代码省略）… */
static int altr_portb_setup(struct altr_edac_device_dev *device)
{
	struct edac_device_ctl_info *dci;
	struct altr_edac_device_dev *altdev;
	char *ecc_name = "sdmmcb-ecc";
	int edac_idx, rc;
	struct device_node *np;
	const struct edac_device_prv_data *prv = &a10_sdmmceccb_data;
	bool is_s10 = device->edac->is_s10;

	rc = altr_check_ecc_deps(device);
	if (rc)
		return rc;

	np = of_find_compatible_node(NULL, NULL, "altr,socfpga-sdmmc-ecc");
	if (!np) {
		edac_printk(KERN_WARNING, EDAC_DEVICE, "SDMMC node not found\n");
		return -ENODEV;
	}

	/* Create the PortB EDAC device */
	edac_idx = edac_device_alloc_index();
	dci = edac_device_alloc_ctl_info(sizeof(*altdev), ecc_name, 1,
					 ecc_name, 1, 0, edac_idx);
	if (!dci) {
		edac_printk(KERN_ERR, EDAC_DEVICE,
			    "%s: Unable to allocate PortB EDAC device\n",
			    ecc_name);
		return -ENOMEM;
	}

	/* Initialize the PortB EDAC device structure from PortA structure */
	altdev = dci->pvt_info;
	*altdev = *device;

	if (!devres_open_group(device->edac->dev, altr_portb_setup, GFP_KERNEL))
		return -ENOMEM;

	/* Update PortB specific values */
	altdev->edac_dev_name = ecc_name;
	altdev->edac_idx = edac_idx;
	altdev->edac_dev = dci;
	altdev->data = prv;
	dci->dev = &altdev->ddev;
	dci->ctl_name = "Altera ECC Manager";
	dci->mod_name = ecc_name;
	dci->dev_name = ecc_name;

	/*
	 * Update the PortB IRQs - A10 has 4, S10 has 2, Index accordingly
	 */

	/* Using compatibles to determine the IRQ Index */
	if (is_s10)
		altdev->sb_irq = irq_of_parse_and_map(np, 1);
	else
		altdev->sb_irq = irq_of_parse_and_map(np, 2);

	if (!altdev->sb_irq) {
		edac_printk(KERN_ERR, EDAC_DEVICE, "Error PortB SBIRQ alloc\n");
		rc = -ENODEV;
		goto err_release_group_1;
	}
	rc = devm_request_irq(device->edac->dev, altdev->sb_irq,
			      prv->ecc_irq_handler, IRQF_TRIGGER_HIGH,
			      ecc_name, altdev);
	if (rc) {
		edac_printk(KERN_ERR, EDAC_DEVICE, "PortB SBERR IRQ error\n");
		goto err_release_group_1;
	}

	if (is_s10) {
		/* Use IRQ to determine SError origin instead of assigning IRQ */
		rc = of_property_read_u32_index(np, "interrupts", 1, &altdev->db_irq);
		if (rc) {
			edac_printk(KERN_ERR, EDAC_DEVICE, "Error PortB DBIRQ alloc\n");
			goto err_release_group_1;
		}
	} else {
		altdev->db_irq = irq_of_parse_and_map(np, 3);
		if (!altdev->db_irq) {
			edac_printk(KERN_ERR, EDAC_DEVICE, "Error PortB DBIRQ alloc\n");
			rc = -ENODEV;
			goto err_release_group_1;
		}
		rc = devm_request_irq(device->edac->dev, altdev->db_irq,
				      prv->ecc_irq_handler, IRQF_TRIGGER_HIGH,
				      ecc_name, altdev);
		if (rc) {
			edac_printk(KERN_ERR, EDAC_DEVICE, "PortB DBERR IRQ error\n");
			goto err_release_group_1;
		}
	}

	rc = edac_device_add_device(dci);
	if (rc) {
		edac_printk(KERN_ERR, EDAC_DEVICE,
			    "edac_device_add_device portB failed\n");
		rc = -ENOMEM;
		goto err_release_group_1;
	}
	altr_create_edacdev_dbgfs(dci, prv);

	list_add(&altdev->next, &altdev->edac->a10_ecc_devices);

	devres_remove_group(device->edac->dev, altr_portb_setup);

	return 0;

err_release_group_1:
	edac_device_free_ctl_info(dci);
	devres_release_group(device->edac->dev, altr_portb_setup);
	edac_printk(KERN_ERR, EDAC_DEVICE,
		    "%s:Error setting up EDAC device: %d\n", ecc_name, rc);
	return rc;
}

static int __init socfpga_init_sdmmc_ecc(struct altr_edac_device_dev *device)
{
	int rc = -ENODEV;
	struct device_node *child;
/* …（同文件无关代码省略）… */
		goto exit;

	/* Setup portB */
	return altr_portb_setup(device);

exit:
	of_node_put(child);
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
	return 0;

err_release_group1:
	edac_device_free_ctl_info(dci);
err_release_group:
	devres_release_group(edac->dev, NULL);
	edac_printk(KERN_ERR, EDAC_DEVICE,
		    "%s:Error setting up EDAC device: %d\n", ecc_name, rc);

/* …（同文件无关代码省略）… */
	.driver = {
		.name = "socfpga_a10_ecc_manager",
		.of_match_table = altr_edac_a10_of_match,
	},
};
module_platform_driver(altr_edac_a10_driver);
