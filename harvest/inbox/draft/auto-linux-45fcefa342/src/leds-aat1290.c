// AUTO-DRAFT from torvalds/linux PR #5b93dc1c0c61fd0fe575cb473e9e25f929fda509
#define AAT1290_MM_TO_FL_RATIO		1000 / 1920
#define AAT1290_MAX_MM_CURRENT(fl_max)	(fl_max * AAT1290_MM_TO_FL_RATIO)
/* …（同文件无关代码省略）… */
	int i, max_mm_current =
			AAT1290_MAX_MM_CURRENT(cfg->max_flash_current);
  // <<< BUG ANCHOR
	led->mm_current_scale = devm_kzalloc(&led->pdev->dev,
					sizeof(max_mm_current_percent),
					GFP_KERNEL);
	if (!led->mm_current_scale)
		return -ENOMEM;
