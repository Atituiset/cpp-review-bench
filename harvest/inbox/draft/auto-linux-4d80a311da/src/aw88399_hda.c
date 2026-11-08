// AUTO-DRAFT from torvalds/linux PR #fd7e9dc9997a3e792e93f8b6795cba1de0f52726
#define AW88399_HDA_I2C_BASE_ADDR	0x34
/* …（同文件无关代码省略）… */
static int aw88399_hda_index_from_i2c(struct aw88399_hda *aw88399)
{
	return to_i2c_client(aw88399->dev)->addr - AW88399_HDA_I2C_BASE_ADDR;
}
/* …（同文件无关代码省略）… */
struct aw88399_prop_model {
	const char *ssid;
	int (*apply_prop)(struct aw88399_hda *aw88399);
};
/* …（同文件无关代码省略）… */
static int aw88399_hda_acpi_probe(struct aw88399_hda *aw88399)
{
	struct acpi_device *adev;
	const char *sub;
	const struct aw88399_prop_model *model;
  // <<< BUG ANCHOR
	aw88399->index = aw88399_hda_index_from_i2c(aw88399);
/* …（同文件无关代码省略）… */
	if (!physdev)
		return -ENODEV;

	sub = acpi_get_subsystem_id(ACPI_HANDLE(physdev));
	if (IS_ERR_OR_NULL(sub)) {
		dev_err(aw88399->dev, "ACPI subsystem ID not available\n");
		return -ENODEV;
	}

	aw88399->acpi_subsystem_id = devm_kstrdup(aw88399->dev, sub, GFP_KERNEL);
	kfree(sub);
	if (!aw88399->acpi_subsystem_id)
		return -ENOMEM;

/* …（同文件无关代码省略）… */
EXPORT_SYMBOL_NS_GPL(aw88399_hda_pm_ops, "SND_HDA_SCODEC_AW88399");

MODULE_DESCRIPTION("AW88399 HDA driver");
MODULE_AUTHOR("Yakov Till <yakov.till@gmail.com>");
MODULE_AUTHOR("Marco Giunta <marco_giunta@outlook.it>");
MODULE_LICENSE("GPL");
