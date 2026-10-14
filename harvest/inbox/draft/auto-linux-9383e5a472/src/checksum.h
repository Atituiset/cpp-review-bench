// AUTO-DRAFT from torvalds/linux PR #b963f2345fa20417401c51a69dedb6a58eea33a9
		"add	%0, %0, r8\n"
		"nor	%0, %0, %0\n"
		: "=r" (sum)
		: "r" (sum << 16), "0" (sum)  // <<< BUG ANCHOR
		: "r8");
	return (__force __sum16) sum;
}
