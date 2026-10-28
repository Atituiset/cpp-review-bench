// AUTO-DRAFT from torvalds/linux PR #59a02493f3b68fd0ad68c661393f680bcca5a85b
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#include <linux/delay.h>
#include <linux/btf.h>

#include <trace/events/sched.h>
#include <trace/syscall.h>

/* …（同文件无关代码省略）… */
#define GFP_TRACE (GFP_KERNEL | __GFP_ZERO)
/* …（同文件无关代码省略）… */
static int __trace_define_field(struct list_head *head, const char *type,
				const char *name, int offset, int size,
				int is_signed, int filter_type, int len,
				int need_test)
{
	struct ftrace_event_field *field;

	field = kmem_cache_alloc(field_cachep, GFP_TRACE);
	if (!field)
		return -ENOMEM;

	field->name = name;
	field->type = type;

	if (filter_type == FILTER_OTHER)
		field->filter_type = filter_assign_type(type);
	else
		field->filter_type = filter_type;

	field->offset = offset;
	field->size = size;
	field->is_signed = is_signed;
	field->needs_test = need_test;
	field->len = len;

	list_add(&field->link, head);

	return 0;
}
/* …（同文件无关代码省略）… */
#define __generic_field(type, item, filter_type)			\
	ret = __trace_define_field(&ftrace_generic_fields, #type,	\
				   #item, 0, 0, is_signed_type(type),	\
				   filter_type, 0, 0);			\
	if (ret)							\
		return ret;
/* …（同文件无关代码省略）… */
	__generic_field(char *, comm, FILTER_COMM);
	__generic_field(char *, stacktrace, FILTER_STACKTRACE);
	__generic_field(char *, STACKTRACE, FILTER_STACKTRACE);

	return ret;
}
/* …（同文件无关代码省略）… */
	struct list_head *head;

	head = trace_get_fields(call);
	/*
	 * head->next points to the last field with the largest offset,
	 * since it was added last by trace_define_field()
/* …（同文件无关代码省略）… */
static int __ftrace_event_enable_disable(struct trace_event_file *file,
					 int enable, int soft_disable)
{
	struct trace_event_call *call = file->event_call;
	struct trace_array *tr = file->tr;
	bool soft_mode = atomic_read(&file->sm_ref) != 0;
	int ret = 0;
	int disable;

	switch (enable) {
	case 0:
		/*
		 * When soft_disable is set and enable is cleared, the sm_ref
		 * reference counter is decremented. If it reaches 0, we want
		 * to clear the SOFT_DISABLED flag but leave the event in the
		 * state that it was. That is, if the event was enabled and
		 * SOFT_DISABLED isn't set, then do nothing. But if SOFT_DISABLED
		 * is set we do not want the event to be enabled before we
		 * clear the bit.
		 *
		 * When soft_disable is not set but the soft_mode is,
		 * we do nothing. Do not disable the tracepoint, otherwise
		 * "soft enable"s (clearing the SOFT_DISABLED bit) won't work.
		 */
		if (soft_disable) {
			if (atomic_dec_return(&file->sm_ref) > 0)
				break;
			disable = file->flags & EVENT_FILE_FL_SOFT_DISABLED;
			soft_mode = false;
			/* Disable use of trace_buffered_event */
			trace_buffered_event_disable();
		} else
			disable = !soft_mode;

		if (disable && (file->flags & EVENT_FILE_FL_ENABLED)) {
			clear_bit(EVENT_FILE_FL_ENABLED_BIT, &file->flags);
			if (file->flags & EVENT_FILE_FL_RECORDED_CMD) {
				tracing_stop_cmdline_record();
				clear_bit(EVENT_FILE_FL_RECORDED_CMD_BIT, &file->flags);
			}

			if (file->flags & EVENT_FILE_FL_RECORDED_TGID) {
				tracing_stop_tgid_record();
				clear_bit(EVENT_FILE_FL_RECORDED_TGID_BIT, &file->flags);
			}

			ret = call->class->reg(call, TRACE_REG_UNREGISTER, file);

			WARN_ON_ONCE(ret);
		}
		/* If in soft mode, just set the SOFT_DISABLE_BIT, else clear it */
		if (soft_mode)
			set_bit(EVENT_FILE_FL_SOFT_DISABLED_BIT, &file->flags);
		else
			clear_bit(EVENT_FILE_FL_SOFT_DISABLED_BIT, &file->flags);
		break;
	case 1:
		/*
		 * When soft_disable is set and enable is set, we want to
		 * register the tracepoint for the event, but leave the event
		 * as is. That means, if the event was already enabled, we do
		 * nothing. If the event is disabled, we set SOFT_DISABLED
		 * before enabling the event tracepoint, so it still seems
		 * to be disabled.
		 */
		if (!soft_disable)
			clear_bit(EVENT_FILE_FL_SOFT_DISABLED_BIT, &file->flags);
		else {
			if (atomic_inc_return(&file->sm_ref) > 1)
				break;
			/* Enable use of trace_buffered_event */
			trace_buffered_event_enable();
		}

		if (!(file->flags & EVENT_FILE_FL_ENABLED)) {
			bool cmd = false, tgid = false;

			/* Keep the event disabled, when going to soft mode. */
			if (soft_disable)
				set_bit(EVENT_FILE_FL_SOFT_DISABLED_BIT, &file->flags);

			if (tr->trace_flags & TRACE_ITER(RECORD_CMD)) {
				cmd = true;
				tracing_start_cmdline_record();
				set_bit(EVENT_FILE_FL_RECORDED_CMD_BIT, &file->flags);
			}

			if (tr->trace_flags & TRACE_ITER(RECORD_TGID)) {
				tgid = true;
				tracing_start_tgid_record();
				set_bit(EVENT_FILE_FL_RECORDED_TGID_BIT, &file->flags);
			}

			ret = call->class->reg(call, TRACE_REG_REGISTER, file);
			if (ret) {
				if (cmd)
					tracing_stop_cmdline_record();
				if (tgid)
					tracing_stop_tgid_record();
				pr_info("event trace: Could not enable event "
					"%s\n", trace_event_name(call));
				break;
			}
			set_bit(EVENT_FILE_FL_ENABLED_BIT, &file->flags);

			/* WAS_ENABLED gets set but never cleared. */
			set_bit(EVENT_FILE_FL_WAS_ENABLED_BIT, &file->flags);
		}
		break;
	}

	return ret;
}
/* …（同文件无关代码省略）… */
static int ftrace_event_enable_disable(struct trace_event_file *file,
				       int enable)
{
	return __ftrace_event_enable_disable(file, enable, 0);
}
/* …（同文件无关代码省略）… */
struct event_mod_load {
	struct list_head	list;
	char			*module;
	char			*match;
	char			*system;
	char			*event;
};

static void free_event_mod(struct event_mod_load *event_mod)
{
	list_del(&event_mod->list);
	kfree(event_mod->module);
	kfree(event_mod->match);
	kfree(event_mod->system);
	kfree(event_mod->event);
	kfree(event_mod);
}

/* …（同文件无关代码省略）… */
}

static int cache_mod(struct trace_array *tr, const char *mod, int set,
		     const char *match, const char *system, const char *event)
{
	struct event_mod_load *event_mod;

/* …（同文件无关代码省略）… */
			goto out_free;
	}

	list_add(&event_mod->list, &tr->mod_events);

	return 0;
/* …（同文件无关代码省略）… */
#else /* CONFIG_MODULES */
static inline void clear_mod_events(struct trace_array *tr) { }
static int cache_mod(struct trace_array *tr, const char *mod, int set,
		     const char *match, const char *system, const char *event)
{
	return -EINVAL;
}
/* …（同文件无关代码省略）… */
static void
event_filter_pid_sched_switch_probe_pre(void *data, bool preempt,
					struct task_struct *prev,
					struct task_struct *next,
					unsigned int prev_state)
{
	struct trace_array *tr = data;
	struct trace_pid_list *no_pid_list;
	struct trace_pid_list *pid_list;
	bool ret;

	pid_list = rcu_dereference_sched(tr->filtered_pids);
	no_pid_list = rcu_dereference_sched(tr->filtered_no_pids);

	/*
	 * Sched switch is funny, as we only want to ignore it
	 * in the notrace case if both prev and next should be ignored.
	 */
	ret = trace_ignore_this_task(NULL, no_pid_list, prev) &&
		trace_ignore_this_task(NULL, no_pid_list, next);

	this_cpu_write(tr->array_buffer.data->ignore_pid, ret ||
		       (trace_ignore_this_task(pid_list, NULL, prev) &&
			trace_ignore_this_task(pid_list, NULL, next)));
}
/* …（同文件无关代码省略）… */
static void
event_filter_pid_sched_switch_probe_post(void *data, bool preempt,
					 struct task_struct *prev,
					 struct task_struct *next,
					 unsigned int prev_state)
{
	struct trace_array *tr = data;
	struct trace_pid_list *no_pid_list;
	struct trace_pid_list *pid_list;

	pid_list = rcu_dereference_sched(tr->filtered_pids);
	no_pid_list = rcu_dereference_sched(tr->filtered_no_pids);

	this_cpu_write(tr->array_buffer.data->ignore_pid,
		       trace_ignore_this_task(pid_list, no_pid_list, next));
}
/* …（同文件无关代码省略）… */
static void
event_filter_pid_sched_wakeup_probe_pre(void *data, struct task_struct *task)
{
	struct trace_array *tr = data;
	struct trace_pid_list *no_pid_list;
	struct trace_pid_list *pid_list;

	/* Nothing to do if we are already tracing */
	if (!this_cpu_read(tr->array_buffer.data->ignore_pid))
		return;

	pid_list = rcu_dereference_sched(tr->filtered_pids);
	no_pid_list = rcu_dereference_sched(tr->filtered_no_pids);

	this_cpu_write(tr->array_buffer.data->ignore_pid,
		       trace_ignore_this_task(pid_list, no_pid_list, task));
}
/* …（同文件无关代码省略）… */
static void
event_filter_pid_sched_wakeup_probe_post(void *data, struct task_struct *task)
{
	struct trace_array *tr = data;
	struct trace_pid_list *no_pid_list;
	struct trace_pid_list *pid_list;

	/* Nothing to do if we are not tracing */
	if (this_cpu_read(tr->array_buffer.data->ignore_pid))
		return;

	pid_list = rcu_dereference_sched(tr->filtered_pids);
	no_pid_list = rcu_dereference_sched(tr->filtered_no_pids);

	/* Set tracing if current is enabled */
	this_cpu_write(tr->array_buffer.data->ignore_pid,
		       trace_ignore_this_task(pid_list, no_pid_list, current));
}
/* …（同文件无关代码省略）… */
static void unregister_pid_events(struct trace_array *tr)
{
	unregister_trace_sched_switch(event_filter_pid_sched_switch_probe_pre, tr);
	unregister_trace_sched_switch(event_filter_pid_sched_switch_probe_post, tr);

	unregister_trace_sched_wakeup(event_filter_pid_sched_wakeup_probe_pre, tr);
	unregister_trace_sched_wakeup(event_filter_pid_sched_wakeup_probe_post, tr);

	unregister_trace_sched_wakeup_new(event_filter_pid_sched_wakeup_probe_pre, tr);
	unregister_trace_sched_wakeup_new(event_filter_pid_sched_wakeup_probe_post, tr);

	unregister_trace_sched_waking(event_filter_pid_sched_wakeup_probe_pre, tr);
	unregister_trace_sched_waking(event_filter_pid_sched_wakeup_probe_post, tr);
}
/* …（同文件无关代码省略）… */
static void __ftrace_clear_event_pids(struct trace_array *tr, int type)
{
	struct trace_pid_list *pid_list;
	struct trace_pid_list *no_pid_list;
	struct trace_event_file *file;
	int cpu;

	pid_list = rcu_dereference_protected(tr->filtered_pids,
					     lockdep_is_held(&event_mutex));
	no_pid_list = rcu_dereference_protected(tr->filtered_no_pids,
					     lockdep_is_held(&event_mutex));

	/* Make sure there's something to do */
	if (!pid_type_enabled(type, pid_list, no_pid_list))
		return;

	if (!still_need_pid_events(type, pid_list, no_pid_list)) {
		unregister_pid_events(tr);

		list_for_each_entry(file, &tr->events, list) {
			clear_bit(EVENT_FILE_FL_PID_FILTER_BIT, &file->flags);
		}

		for_each_possible_cpu(cpu)
			per_cpu_ptr(tr->array_buffer.data, cpu)->ignore_pid = false;
	}

	if (type & TRACE_PIDS)
		rcu_assign_pointer(tr->filtered_pids, NULL);

	if (type & TRACE_NO_PIDS)
		rcu_assign_pointer(tr->filtered_no_pids, NULL);

	/* Wait till all users are no longer using pid filtering */
	tracepoint_synchronize_unregister();

	if ((type & TRACE_PIDS) && pid_list)
		trace_pid_list_free(pid_list);

	if ((type & TRACE_NO_PIDS) && no_pid_list)
		trace_pid_list_free(no_pid_list);
}
/* …（同文件无关代码省略）… */
static int
__ftrace_set_clr_event_nolock(struct trace_array *tr, const char *match,
			      const char *sub, const char *event, int set,
			      const char *mod)
{
	struct trace_event_file *file;
	struct trace_event_call *call;
/* …（同文件无关代码省略）… */
		if (event && strcmp(event, name) != 0)
			continue;

		ret = ftrace_event_enable_disable(file, set);

		/*
		 * Save the first error and return that. Some events
/* …（同文件无关代码省略）… */
	 * check if the module was loaded. If it wasn't cache it.
	 */
	if (module && ret == -EINVAL && !eret)
		ret = cache_mod(tr, module, set, match, sub, event);

	return ret;
}

static int __ftrace_set_clr_event(struct trace_array *tr, const char *match,
				  const char *sub, const char *event, int set,
				  const char *mod)
{
	int ret;

	if (trace_array_is_readonly(tr))
		return -EACCES;

	mutex_lock(&event_mutex);
	ret = __ftrace_set_clr_event_nolock(tr, match, sub, event, set, mod);
	mutex_unlock(&event_mutex);

	return ret;
}

int ftrace_set_clr_event(struct trace_array *tr, char *buf, int set)
{
	char *event = NULL, *sub = NULL, *match, *mod;
	int ret;

	if (!tr)
		return -ENOENT;

	/* Modules events can be appended with :mod:<module> */
	mod = strstr(buf, ":mod:");
	if (mod) {
		*mod = '\0';
		/* move to the module name */
		mod += 5;
	}

	/*
/* …（同文件无关代码省略）… */
	 *  the name <name> or any event that matches <name>
	 */

	match = strsep(&buf, ":");
	if (buf) {
		sub = match;
		event = buf;
		match = NULL;

		if (!strlen(sub) || strcmp(sub, "*") == 0)
			sub = NULL;
		if (!strlen(event) || strcmp(event, "*") == 0)
			event = NULL;
	} else if (mod) {
		/* Allow wildcard for no length or star */
		if (!strlen(match) || strcmp(match, "*") == 0)
			match = NULL;
	}

	ret = __ftrace_set_clr_event(tr, match, sub, event, set, mod);

	/* Put back the colon to allow this to be called again */
	if (buf)
		*(buf - 1) = ':';
	if (mod)
		*(mod - 5) = ':';

	return ret;
}

/**
/* …（同文件无关代码省略）… */
int trace_set_clr_event(const char *system, const char *event, int set)
{
	struct trace_array *tr = top_trace_array();

	if (!tr)
		return -ENODEV;

	return __ftrace_set_clr_event(tr, NULL, system, event, set, NULL);
}
EXPORT_SYMBOL_GPL(trace_set_clr_event);

/* …（同文件无关代码省略）… */
int trace_array_set_clr_event(struct trace_array *tr, const char *system,
		const char *event, bool enable)
{
	int set;

	if (!tr)
		return -ENOENT;

	set = (enable == true) ? 1 : 0;
	return __ftrace_set_clr_event(tr, NULL, system, event, set, NULL);
}
EXPORT_SYMBOL_GPL(trace_array_set_clr_event);

/* …（同文件无关代码省略）… */
		if (*parser.buffer == '!')
			set = 0;

		ret = ftrace_set_clr_event(tr, parser.buffer + !set, set);
		if (ret)
			goto out_put;
	}
/* …（同文件无关代码省略）… */
	if (system)
		name = system->name;

	ret = __ftrace_set_clr_event(dir->tr, NULL, name, NULL, val, NULL);
	if (ret)
		goto out;

/* …（同文件无关代码省略）… */

		__ftrace_set_clr_event_nolock(tr, event_mod->match,
					      event_mod->system,
					      event_mod->event, 1, mod->name);
		free_event_mod(event_mod);
	}
}
/* …（同文件无关代码省略）… */
	__ftrace_clear_event_pids(tr, TRACE_PIDS | TRACE_NO_PIDS);

	/* Disable any running events */
	__ftrace_set_clr_event_nolock(tr, NULL, NULL, NULL, 0, NULL);

	/* Make sure no more events are being executed */
	tracepoint_synchronize_unregister();
/* …（同文件无关代码省略）… */
	while ((token = strsep(&buf, ","))) {
		if (*token) {
			if (enable) {
				if (ftrace_set_clr_event(tr, token, 1))
					pr_warn("Failed to enable trace event: %s\n", token);
			} else {
				ftrace_set_clr_event(tr, token, 0);
			}
		}

/* …（同文件无关代码省略）… */
static __init void test_work(struct work_struct *dummy)
{
	spin_lock(&test_spinlock);
	spin_lock_irq(&test_spinlock_irq);
	udelay(1);
	spin_unlock_irq(&test_spinlock_irq);
	spin_unlock(&test_spinlock);

	mutex_lock(&test_mutex);
	msleep(1);
	mutex_unlock(&test_mutex);
}
/* …（同文件无关代码省略）… */
static __init int event_test_thread(void *unused)
{
	void *test_malloc;

	test_malloc = kmalloc(1234, GFP_KERNEL);
	if (!test_malloc)
		pr_info("failed to kmalloc\n");

	schedule_on_each_cpu(test_work);

	kfree(test_malloc);

	set_current_state(TASK_INTERRUPTIBLE);
	while (!kthread_should_stop()) {
		schedule();
		set_current_state(TASK_INTERRUPTIBLE);
	}
	__set_current_state(TASK_RUNNING);

	return 0;
}
/* …（同文件无关代码省略）… */
static __init void event_test_stuff(void)
{
	struct task_struct *test_thread;

	test_thread = kthread_run(event_test_thread, NULL, "test-events");
	if (WARN_ON(IS_ERR(test_thread)))
		return;
	msleep(1);
	kthread_stop(test_thread);
}
/* …（同文件无关代码省略）… */

		pr_info("Testing event system %s: ", system->name);

		ret = __ftrace_set_clr_event(tr, NULL, system->name, NULL, 1, NULL);
		if (WARN_ON_ONCE(ret)) {
			pr_warn("error enabling system %s\n",
				system->name);
/* …（同文件无关代码省略）… */

		event_test_stuff();

		ret = __ftrace_set_clr_event(tr, NULL, system->name, NULL, 0, NULL);
		if (WARN_ON_ONCE(ret)) {
			pr_warn("error disabling system %s\n",
				system->name);
/* …（同文件无关代码省略）… */
	pr_info("Running tests on all trace events:\n");
	pr_info("Testing all events: ");

	ret = __ftrace_set_clr_event(tr, NULL, NULL, NULL, 1, NULL);
	if (WARN_ON_ONCE(ret)) {
		pr_warn("error enabling all events\n");
		return;
/* …（同文件无关代码省略）… */
	event_test_stuff();

	/* reset sysname */
	ret = __ftrace_set_clr_event(tr, NULL, NULL, NULL, 0, NULL);
	if (WARN_ON_ONCE(ret)) {
		pr_warn("error disabling all events\n");
		return;
