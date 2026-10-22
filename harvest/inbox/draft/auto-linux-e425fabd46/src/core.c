// AUTO-DRAFT from torvalds/linux PR #7be4bc52fd9e0e55dc9a2e0232b89427d2c4b614
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
bool mce_available(struct cpuinfo_x86 *c)
{
	if (mca_cfg.disabled)
		return false;
	return cpu_has(c, X86_FEATURE_MCE) && cpu_has(c, X86_FEATURE_MCA);
}
/* …（同文件无关代码省略）… */
static void mce_schedule_work(void)
{
	if (!mce_gen_pool_empty())
		schedule_work(&mce_work);
}
/* …（同文件无关代码省略）… */
void machine_check_poll(enum mcp_flags flags, mce_banks_t *b)
{
	struct mce_bank *mce_banks = this_cpu_ptr(mce_banks_array);
	struct mce_hw_err err;
	struct mce *m;
	int i;

	inc_irq_stat(MCE_POLL);

	mce_gather_info(&err, NULL);
	m = &err.m;

	if (flags & MCP_TIMESTAMP)
		m->tsc = rdtsc();

	for (i = 0; i < this_cpu_read(mce_num_banks); i++) {
		if (!mce_banks[i].ctl || !test_bit(i, *b))
			continue;

		m->misc = 0;
		m->addr = 0;
		m->bank = i;

		barrier();
		m->status = mce_rdmsrq(mca_msr_reg(i, MCA_STATUS));

		/*
		 * Update storm tracking here, before checking for the
		 * MCI_STATUS_VAL bit. Valid corrected errors count
		 * towards declaring, or maintaining, storm status. No
		 * error in a bank counts towards avoiding, or ending,
		 * storm status.
		 */
		if (!mca_cfg.cmci_disabled)
			mce_track_storm(m);

		/* Verify that the error should be logged based on hardware conditions. */
		if (!should_log_poll_error(flags, &err))
			continue;

		mce_read_aux(&err, i);
		m->severity = mce_severity(m, NULL, NULL, false);
		/*
		 * Don't get the IP here because it's unlikely to
		 * have anything to do with the actual error location.
		 */

		if (mca_cfg.dont_log_ce && !mce_usable_address(m))
			goto clear_it;

		if (flags & MCP_QUEUE_LOG)
			mce_gen_pool_add(&err);
		else
			mce_log(&err);

clear_it:
		clear_bank(m);
	}

	/*
	 * Don't clear MCG_STATUS here because it's only defined for
	 * exceptions.
	 */

	sync_core();
}
/* …（同文件无关代码省略）… */
static void __start_timer(struct timer_list *t, unsigned long interval)
{
	unsigned long when = jiffies + interval;
	unsigned long flags;

	local_irq_save(flags);

	if (!timer_pending(t) || time_before(when, t->expires))
		mod_timer(t, round_jiffies(when));

	local_irq_restore(flags);
}
/* …（同文件无关代码省略）… */
static bool should_enable_timer(unsigned long iv)
{
	return !mca_cfg.ignore_ce && iv;
}
/* …（同文件无关代码省略）… */
static void mce_timer_fn(struct timer_list *t)
{
	struct timer_list *cpu_t = this_cpu_ptr(&mce_timer);
	unsigned long iv;

	WARN_ON(cpu_t != t);

	iv = __this_cpu_read(mce_next_interval);

	if (mce_available(this_cpu_ptr(&cpu_info)))
		mc_poll_banks();

	/*
	 * Alert userspace if needed. If we logged an MCE, reduce the polling
	 * interval, otherwise increase the polling interval.
	 */
	if (!mce_gen_pool_empty())
		iv = max(iv / 2, (unsigned long) HZ/100);
	else
		iv = min(iv * 2, round_jiffies_relative(check_interval * HZ));

	if (mce_get_storm_mode()) {
		__start_timer(t, HZ);
	} else if (should_enable_timer(iv)) {
		__this_cpu_write(mce_next_interval, iv);
		__start_timer(t, iv);
	}
}
/* …（同文件无关代码省略）… */
static void mce_timer_delete_all(void)
{
	int cpu;

	for_each_online_cpu(cpu)
		timer_delete_sync(&per_cpu(mce_timer, cpu));
}
/* …（同文件无关代码省略）… */
static void __mcheck_cpu_init_generic(void)
{
	u64 cap;

	rdmsrq(MSR_IA32_MCG_CAP, cap);
	if (cap & MCG_CTL_P)
		wrmsrq(MSR_IA32_MCG_CTL, ~0ULL);
}
/* …（同文件无关代码省略）… */
static void __mcheck_cpu_init_prepare_banks(void)
{
	struct mce_bank *mce_banks = this_cpu_ptr(mce_banks_array);
	u64 msrval;
	int i;

	/*
	 * Log the machine checks left over from the previous reset. Log them
	 * only, do not start processing them. That will happen in mcheck_late_init()
	 * when all consumers have been registered on the notifier chain.
	 */
	if (mca_cfg.bootlog) {
		mce_banks_t all_banks;

		bitmap_fill(all_banks, MAX_NR_BANKS);
		machine_check_poll(MCP_UC | MCP_QUEUE_LOG, &all_banks);
	}

	for (i = 0; i < this_cpu_read(mce_num_banks); i++) {
		struct mce_bank *b = &mce_banks[i];

		if (!b->init)
			continue;

		wrmsrq(mca_msr_reg(i, MCA_CTL), b->ctl);
		wrmsrq(mca_msr_reg(i, MCA_STATUS), 0);

		rdmsrq(mca_msr_reg(i, MCA_CTL), msrval);
		b->init = !!msrval;
	}
}
/* …（同文件无关代码省略）… */
static void mce_start_timer(struct timer_list *t)
{
	unsigned long iv = check_interval * HZ;

	if (should_enable_timer(iv)) {
		this_cpu_write(mce_next_interval, iv);
		__start_timer(t, iv);
	}
}
/* …（同文件无关代码省略）… */
static void __mcheck_cpu_init_timer(void)
{
	struct timer_list *t = this_cpu_ptr(&mce_timer);

	timer_setup(t, mce_timer_fn, TIMER_PINNED);
	mce_start_timer(t);
}
/* …（同文件无关代码省略）… */
static void mce_cpu_restart(void *data)
{
	if (!mce_available(raw_cpu_ptr(&cpu_info)))
		return;
	__mcheck_cpu_init_generic();
	__mcheck_cpu_init_prepare_banks();
	__mcheck_cpu_init_timer();
	cr4_set_bits(X86_CR4_MCE);
}
/* …（同文件无关代码省略）… */
static void mce_restart(void)
{
	mce_timer_delete_all();
	on_each_cpu(mce_cpu_restart, NULL, 1);
	mce_schedule_work();
}
/* …（同文件无关代码省略）… */
static void mce_disable_cmci(void *data)
{
	if (!mce_available(raw_cpu_ptr(&cpu_info)))
		return;
	cmci_clear();
}
/* …（同文件无关代码省略）… */
static void mce_enable_ce(void *all)
{
	if (!mce_available(raw_cpu_ptr(&cpu_info)))
		return;
	cmci_reenable();
	cmci_recheck();
	if (all)
		__mcheck_cpu_init_timer();
}
/* …（同文件无关代码省略）… */
static ssize_t set_ignore_ce(struct device *s,
			     struct device_attribute *attr,
			     const char *buf, size_t size)
{
	u64 new;

	if (kstrtou64(buf, 0, &new) < 0)
		return -EINVAL;

	mutex_lock(&mce_sysfs_mutex);
	if (mca_cfg.ignore_ce ^ !!new) {
		if (new) {
			/* disable ce features */
			mce_timer_delete_all();
			on_each_cpu(mce_disable_cmci, NULL, 1);
			mca_cfg.ignore_ce = true;
		} else {
			/* enable ce features */
			mca_cfg.ignore_ce = false;
			on_each_cpu(mce_enable_ce, (void *)1, 1);
		}
	}
	mutex_unlock(&mce_sysfs_mutex);

	return size;
}
/* …（同文件无关代码省略）… */
static ssize_t set_cmci_disabled(struct device *s,
				 struct device_attribute *attr,
				 const char *buf, size_t size)
{
	u64 new;

	if (kstrtou64(buf, 0, &new) < 0)
		return -EINVAL;

	mutex_lock(&mce_sysfs_mutex);
	if (mca_cfg.cmci_disabled ^ !!new) {
		if (new) {
			/* disable cmci */
			on_each_cpu(mce_disable_cmci, NULL, 1);
			mca_cfg.cmci_disabled = true;
		} else {
			/* enable cmci */
			mca_cfg.cmci_disabled = false;
			on_each_cpu(mce_enable_ce, NULL, 1);
		}
	}
	mutex_unlock(&mce_sysfs_mutex);

	return size;
}
/* …（同文件无关代码省略）… */
static ssize_t store_int_with_restart(struct device *s,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	unsigned long old_check_interval = check_interval;
	ssize_t ret = device_store_ulong(s, attr, buf, size);

	if (check_interval == old_check_interval)
		return ret;

	mutex_lock(&mce_sysfs_mutex);
	mce_restart();
	mutex_unlock(&mce_sysfs_mutex);

	return ret;
}
/* …（同文件无关代码省略）… */
static DEVICE_BOOL_ATTR(print_all, 0644, mca_cfg.print_all);

static struct dev_ext_attribute dev_attr_check_interval = {
	__ATTR(check_interval, 0644, device_show_int, store_int_with_restart),
	&check_interval
};

static struct dev_ext_attribute dev_attr_ignore_ce = {
	__ATTR(ignore_ce, 0644, device_show_bool, set_ignore_ce),
	&mca_cfg.ignore_ce
};

static struct dev_ext_attribute dev_attr_cmci_disabled = {
	__ATTR(cmci_disabled, 0644, device_show_bool, set_cmci_disabled),
	&mca_cfg.cmci_disabled
};
