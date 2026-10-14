// AUTO-DRAFT from torvalds/linux PR #4056b3c9d3c92ea45955641dbafeb9b960e146ef
		"add	%0, %0, r8\n"
		"nor	%0, %0, %0\n"
		: "=r" (sum)
		: "r" (sum << 16), "0" (sum)  // <<< BUG ANCHOR
		: "r8");
	return (__force __sum16) sum;
}
