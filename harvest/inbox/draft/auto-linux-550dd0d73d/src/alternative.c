// AUTO-DRAFT from torvalds/linux PR #53cbbb09c1b87425cd7184f81ecdb830aa3b0b87
	"fineibt_paranoid_start:			\n"
	"	mov	$0x12345678, %eax		\n"
	"	cmpl	-11(%r11), %eax			\n"
	"	cs lea	-0x10(%r11), %r11		\n"  // <<< BUG ANCHOR
	"#fineibt_caller_size:                          \n"
	"	jne	fineibt_paranoid_start+0xd	\n"
	"fineibt_paranoid_ind:				\n"
