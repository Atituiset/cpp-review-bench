// AUTO-DRAFT from torvalds/linux PR #f7bec60381da152e110bb71358c0903f9bc1826e
	if (video_is_registered(&v4l2->radio_dev)) {
		dev_info(&dev->intf->dev, "V4L2 device %s deregistered\n",
			 video_device_node_name(&v4l2->radio_dev));
		vb2_video_unregister_device(&v4l2->radio_dev);  // <<< BUG ANCHOR
	}
	if (video_is_registered(&v4l2->vbi_dev)) {
		dev_info(&dev->intf->dev, "V4L2 device %s deregistered\n",
/* …（同文件无关代码省略）… */
		dev_info(&dev->intf->dev,
			 "V4L2 device %s deregistered\n",
			 video_device_node_name(&v4l2->radio_dev));
		vb2_video_unregister_device(&v4l2->radio_dev);
	}
	if (video_is_registered(&v4l2->vbi_dev)) {
		dev_info(&dev->intf->dev,
