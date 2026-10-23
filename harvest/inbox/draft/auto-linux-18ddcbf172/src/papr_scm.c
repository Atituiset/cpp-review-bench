// AUTO-DRAFT from torvalds/linux PR #864c07de0c7dcbca58b61625dfa2e461ae63b242
  // <<< BUG ANCHOR
free_stats:
	kfree(stats);
	return rc ? rc : (ssize_t)seq_buf_used(&s);
}
static DEVICE_ATTR_ADMIN_RO(perf_stats);

/* …（同文件无关代码省略）… */
	if (health & PAPR_PMEM_SCRUBBED_AND_LOCKED)
		seq_buf_printf(&s, "scrubbed locked ");

	if (seq_buf_used(&s))
		seq_buf_printf(&s, "\n");

	return seq_buf_used(&s);
}
DEVICE_ATTR_RO(flags);
