// AUTO-DRAFT from torvalds/linux PR #7cd8495c0f16275c4f9abe1cef09a7b5fcd6d046
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>

static const struct btf_type *
io_lookup_struct_type(struct btf *btf, const char *name)
{
	s32 type_id;

	type_id = btf_find_by_name_kind(btf, name, BTF_KIND_STRUCT);
	if (type_id < 0)
		return NULL;
	return btf_type_by_id(btf, type_id);
}
/* …（同文件无关代码省略）… */
static int bpf_io_init(struct btf *btf)
{
	int ret;

	loop_params_type = io_lookup_struct_type(btf, "iou_loop_params");
	if (!loop_params_type) {
		pr_err("io_uring: Failed to locate iou_loop_params\n");
		return -EINVAL;
	}

	ret = register_btf_kfunc_id_set(BPF_PROG_TYPE_STRUCT_OPS,
					&bpf_io_uring_kfunc_set);
	if (ret) {
		pr_err("io_uring: Failed to register kfuncs (%d)\n", ret);
		return ret;
	}
	return 0;
}
/* …（同文件无关代码省略）… */
static int bpf_io_check_member(const struct btf_type *t,
				const struct btf_member *member,
				const struct bpf_prog *prog)
{
	return 0;
}

static int bpf_io_init_member(const struct btf_type *t,
			       const struct btf_member *member,
			       void *kdata, const void *udata)
{
	u32 moff = __btf_member_bit_offset(t, member) / 8;
	const struct io_uring_bpf_ops *uops = udata;
	struct io_uring_bpf_ops *ops = kdata;

	switch (moff) {
	case offsetof(struct io_uring_bpf_ops, ring_fd):
		ops->ring_fd = uops->ring_fd;
		return 1;
	}
	return 0;
}
/* …（同文件无关代码省略）… */
	.check_member = bpf_io_check_member,
	.init_member = bpf_io_init_member,
	.init = bpf_io_init,
	.cfi_stubs = &io_bpf_ops_stubs,
	.name = "io_uring_bpf_ops",
	.owner = THIS_MODULE,
