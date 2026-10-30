// AUTO-DRAFT from torvalds/linux PR #46806af187f448b0d5481f284e7c900a57ee6a6d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static void test_st_ops_assoc_in_timer(void)
{
	struct struct_ops_assoc_in_timer *skel = NULL;
	int err;

	skel = struct_ops_assoc_in_timer__open_and_load();
	if (!ASSERT_OK_PTR(skel, "struct_ops_assoc_in_timer__open"))
		goto out;

	err = bpf_program__assoc_struct_ops(skel->progs.syscall_prog,
					    skel->maps.st_ops_map, NULL);
	ASSERT_OK(err, "bpf_program__assoc_struct_ops");

	err = struct_ops_assoc_in_timer__attach(skel);
	if (!ASSERT_OK(err, "struct_ops_assoc__attach"))
		goto out;

	/*
	 * Run .test_1 by calling kfunc bpf_kfunc_multi_st_ops_test_1_prog_arg() and checks
	 * the return value. .test_1 will also schedule timer_cb that runs .test_1 again
	 * immediately.
	 */
	err = bpf_prog_test_run_opts(bpf_program__fd(skel->progs.syscall_prog), NULL);
	ASSERT_OK(err, "bpf_prog_test_run_opts");

	/* Check the return of the kfunc after timer_cb runs */
	while (!READ_ONCE(skel->bss->timer_cb_run))
		sched_yield();
	ASSERT_EQ(skel->bss->timer_test_1_ret, 1234, "skel->bss->timer_test_1_ret");
	ASSERT_EQ(skel->bss->test_err, 0, "skel->bss->test_err_a");
out:
	struct_ops_assoc_in_timer__destroy(skel);
}

static void test_st_ops_assoc_in_timer_no_uref(void)
{
	struct struct_ops_assoc_in_timer *skel = NULL;
	struct bpf_link *link;
/* …（同文件无关代码省略）… */

	err = bpf_program__assoc_struct_ops(skel->progs.syscall_prog,
					    skel->maps.st_ops_map, NULL);
	ASSERT_OK(err, "bpf_program__assoc_struct_ops");

	link = bpf_map__attach_struct_ops(skel->maps.st_ops_map);
	if (!ASSERT_OK_PTR(link, "bpf_map__attach_struct_ops"))
/* …（同文件无关代码省略）… */
	/*
	 * Run .test_1 by calling kfunc bpf_kfunc_multi_st_ops_test_1_prog_arg() and checks
	 * the return value. .test_1 will also schedule timer_cb that runs .test_1 again.
	 * timer_cb will run 500ms after syscall_prog runs, when the user space no longer
	 * holds a reference to st_ops_map.
	 */
	skel->bss->timer_ns = 500000000;
	err = bpf_prog_test_run_opts(bpf_program__fd(skel->progs.syscall_prog), NULL);
	ASSERT_OK(err, "bpf_prog_test_run_opts");

	/* Detach and close struct_ops map to cause it to be freed */
	bpf_link__destroy(link);
/* …（同文件无关代码省略）… */
	struct_ops_assoc_in_timer__destroy(skel);
}

void test_struct_ops_assoc(void)
{
	if (test__start_subtest("st_ops_assoc"))
/* …（同文件无关代码省略）… */
		test_st_ops_assoc_in_timer();
	if (test__start_subtest("st_ops_assoc_in_timer_no_uref"))
		test_st_ops_assoc_in_timer_no_uref();
}
