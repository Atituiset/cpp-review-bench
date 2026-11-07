// AUTO-DRAFT from torvalds/linux PR #d48723711bae9b8cdc56719c448ba42fef28cbee
					struct sdei_registered_event *arg);  // <<< BUG ANCHOR

unsigned long do_sdei_event(struct pt_regs *regs,
			    struct sdei_registered_event *arg);

unsigned long sdei_arch_get_entry_point(int conduit);
#define sdei_arch_get_entry_point(x)	sdei_arch_get_entry_point(x)
