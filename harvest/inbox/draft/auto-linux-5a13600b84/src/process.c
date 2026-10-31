// AUTO-DRAFT from torvalds/linux PR #6e07889d8fedf729282ed14ff202e06dbb67444b
	return 0;
}

#else

static void flush_gcs(void) { }
static int copy_thread_gcs(struct task_struct *p,
			   const struct kernel_clone_args *args)
{
/* …（同文件无关代码省略）… */
	flush_gcs();
}

void arch_release_task_struct(struct task_struct *tsk)
{
	fpsimd_release_task(tsk);
/* …（同文件无关代码省略）… */

		p->thread.cpu_context.x19 = (unsigned long)args->fn;
		p->thread.cpu_context.x20 = (unsigned long)args->fn_arg;

		if (system_supports_poe())
			p->thread.por_el0 = POR_EL0_INIT;
