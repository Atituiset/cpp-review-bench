// AUTO-DRAFT from torvalds/linux PR #c44104f3642da720164042623e4832836aefdd8e
	if (err)
		return dev_err_probe(dev, err, "failed to get resets for core %d\n", core->index);

	err = devm_clk_bulk_get(dev, ARRAY_SIZE(core->clks), core->clks);
	if (err)
		return dev_err_probe(dev, err, "failed to get clocks for core %d\n", core->index);
