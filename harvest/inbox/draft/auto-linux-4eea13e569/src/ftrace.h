// AUTO-DRAFT from torvalds/linux PR #2bad4ba5bd4ed7075c7e874cd30c52e07a74c8cc
	stackp = (unsigned long *)ftrace_regs_get_stack_pointer(fregs);
	if (((unsigned long)(stackp + nth) & ~(THREAD_SIZE - 1)) ==
	    ((unsigned long)stackp & ~(THREAD_SIZE - 1)))
		return *(stackp + nth);  // <<< BUG ANCHOR

	return 0;
}
