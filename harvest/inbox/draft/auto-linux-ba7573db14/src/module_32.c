// AUTO-DRAFT from torvalds/linux PR #7ccb044c27720d48175577080b851f047ddcfd1c

#ifdef CONFIG_DYNAMIC_FTRACE
	_count_relocs++;	/* add one for ftrace_caller */
#endif
	return _count_relocs;
}
