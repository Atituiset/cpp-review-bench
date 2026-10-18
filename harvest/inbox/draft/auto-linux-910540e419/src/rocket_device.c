// AUTO-DRAFT from torvalds/linux PR #58161ead29e50d70a317ea26b472e0c7508e69da
		if (of_device_is_available(core_node))
			num_cores++;

	rdev->cores = devm_kcalloc(dev, num_cores, sizeof(*rdev->cores), GFP_KERNEL);
	if (!rdev->cores)
		return ERR_PTR(-ENOMEM);
