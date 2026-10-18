// AUTO-DRAFT from torvalds/linux PR #58161ead29e50d70a317ea26b472e0c7508e69da
		/* First core probing, initialize DRM device. */
		rdev = rocket_device_init(drm_dev, &rocket_drm_driver);
		if (IS_ERR(rdev)) {
			dev_err(&pdev->dev, "failed to initialize rocket device\n");
			return PTR_ERR(rdev);  // <<< BUG ANCHOR
		}
	}

/* …（同文件无关代码省略）… */
{
	struct rocket_device *rdev = dev_get_drvdata(dev);

	for (unsigned int core = 0; core < rdev->num_cores; core++) {
		if (dev == rdev->cores[core].dev)
			return core;
	}
