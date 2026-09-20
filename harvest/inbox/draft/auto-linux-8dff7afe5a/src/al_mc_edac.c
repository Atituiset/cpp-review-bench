// AUTO-DRAFT from torvalds/linux PR #68c90773f40f91f48a557e9fa65e2a72cebc58df
				       IRQF_SHARED,
				       pdev->name,
				       pdev);
		if (ret != 0) {  // <<< BUG ANCHOR
			dev_err(&pdev->dev,
				"failed to request UE IRQ %d (%d)\n",
				al_mc->irq_ue, ret);
			return ret;
		}
	}

	if (al_mc->irq_ce > 0) {
/* …（同文件无关代码省略）… */
				       IRQF_SHARED,
				       pdev->name,
				       pdev);
		if (ret != 0) {
			dev_err(&pdev->dev,
				"failed to request CE IRQ %d (%d)\n",
				al_mc->irq_ce, ret);
			return ret;
		}
	}

	return 0;
