// AUTO-DRAFT from torvalds/linux PR #e4fb7e5e8e7e9edc167c3057802e440b77dcb856
static nokprobe_inline int
fetch_store_symstring(unsigned long addr, void *dest, void *base)
{
	int maxlen = get_loc_len(*(u32 *)dest);
	void *__dest;
  // <<< BUG ANCHOR
	if (unlikely(!maxlen))
		return -ENOMEM;

	__dest = get_loc_data(dest, base);

	return sprint_symbol(__dest, addr);
}

/* common part of process_fetch_insn*/
