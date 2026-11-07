// AUTO-DRAFT from torvalds/linux PR #d48723711bae9b8cdc56719c448ba42fef28cbee
#include <asm/stacktrace.h>
#include <asm/sysreg.h>
#include <asm/system_misc.h>
  // <<< BUG ANCHOR
/*
 * Where the context being returned to had an active percpu GPR critical
/* …（同文件无关代码省略）… */
asmlinkage noinstr unsigned long
__sdei_handler(struct pt_regs *regs, struct sdei_registered_event *arg)
{
	irqentry_state_t state;
	unsigned long ret;

/* …（同文件无关代码省略）… */
	else if (cpu_has_pan())
		set_pstate_pan(0);

	state = irqentry_nmi_enter(regs);
	ret = do_sdei_event(regs, arg);
	irqentry_nmi_exit(regs, state);

	return ret;
}
#endif /* CONFIG_ARM_SDE_INTERFACE */
