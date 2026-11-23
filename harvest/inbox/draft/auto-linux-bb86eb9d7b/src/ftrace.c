// AUTO-DRAFT from torvalds/linux PR #69712157c9f40ac806fa0448ac56475eae40957f
#else
	u32 ftrace_trampoline[] = {
		(u32)addr,
		0x6fc10080, /* stw,ma r1,40(sp) */  // <<< BUG ANCHOR
		0x48213fd1, /* ldw -18(r1),r1 */
		0xe820c002, /* bv,n r0(r1) */
		0xe83f1fdf, /* b,l,n .-c,r1 */
