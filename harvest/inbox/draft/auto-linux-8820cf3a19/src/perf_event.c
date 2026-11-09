// AUTO-DRAFT from torvalds/linux PR #e0d8e8de8bcb1ef7b749033b5af5518ba8df7bec
  // <<< BUG ANCHOR
static int arc_pmu_raw_alloc(struct device *dev)
{
	arc_pmu->attr = devm_kmalloc_array(dev, arc_pmu->n_events + 1,
		sizeof(*arc_pmu->attr), GFP_KERNEL | __GFP_ZERO);
	if (!arc_pmu->attr)
		return -ENOMEM;

	arc_pmu->attrs = devm_kmalloc_array(dev, arc_pmu->n_events + 1,
		sizeof(*arc_pmu->attrs), GFP_KERNEL | __GFP_ZERO);
	if (!arc_pmu->attrs)
		return -ENOMEM;

	arc_pmu->raw_entry = devm_kmalloc_array(dev, arc_pmu->n_events,
		sizeof(*arc_pmu->raw_entry), GFP_KERNEL | __GFP_ZERO);
	if (!arc_pmu->raw_entry)
		return -ENOMEM;

/* …（同文件无关代码省略）… */
	if (WARN(!cc_bcr.v, "Counters exist but No countable conditions?"))
		return -EINVAL;

	arc_pmu = devm_kzalloc(&pdev->dev, sizeof(struct arc_pmu), GFP_KERNEL);
	if (!arc_pmu)
		return -ENOMEM;
