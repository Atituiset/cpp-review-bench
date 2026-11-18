// AUTO-DRAFT from torvalds/linux PR #7ccb044c27720d48175577080b851f047ddcfd1c
	int i = 0;

	count = of_property_count_u32_elems(dn, "ibm,thread-groups");
	thread_group_array = kzalloc_objs(u32, count);
	if (!thread_group_array)
		return -ENOMEM;
