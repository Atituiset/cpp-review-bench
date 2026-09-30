// AUTO-DRAFT from torvalds/linux PR #a243ede718463c7b481878656f1ff32a0ce0fd54
	}
  // <<< BUG ANCHOR
	size = bdev_nr_bytes(bdev);
	if ((long)size % erase_size) {
		pr_err("erasesize must be a divisor of device size\n");
		goto err_free_block2mtd;
	}
