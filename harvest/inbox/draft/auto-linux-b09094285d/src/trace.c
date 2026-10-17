// AUTO-DRAFT from torvalds/linux PR #e2c278bc37b52222e53d5f88971ffcfacd6c97be
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
	struct tracer_flags	*flags;
};

/*
 * To prevent the comm cache from being overwritten when no
 * tracing is active, only save the comm when a trace event
/* …（同文件无关代码省略）… */
static void trace_array_kick_autoremove(struct trace_array *tr)
{
	if (autoremove_wq)
		queue_work(autoremove_wq, &tr->autoremove_work);
}
/* …（同文件无关代码省略）… */
static void __trace_array_put(struct trace_array *this_tr)
{
	WARN_ON(!this_tr->ref);
	this_tr->ref--;
	/*
	 * When free_on_close is set, prepare removing the array
	 * when the last reference is released.
	 */
	if (this_tr->ref == 1 && this_tr->free_on_close)
		trace_array_kick_autoremove(this_tr);
}
/* …（同文件无关代码省略）… */
static u64 buffer_ftrace_now(struct array_buffer *buf, int cpu)
{
	u64 ts;

	/* Early boot up does not have a buffer yet */
	if (!buf->buffer)
		return trace_clock_local();

	ts = ring_buffer_time_stamp(buf->buffer);
	ring_buffer_normalize_time_stamp(buf->buffer, cpu, &ts);

	return ts;
}
/* …（同文件无关代码省略）… */
	tracing_reset_online_cpus(&tr->array_buffer);

	tr->current_trace = type;
	tr->current_trace_flags = type->flags ? : type->default_flags;

#ifdef CONFIG_TRACER_MAX_TRACE
	if (tracer_uses_snapshot(type)) {
/* …（同文件无关代码省略）… */
void tracing_reset_online_cpus(struct array_buffer *buf)
{
	struct trace_buffer *buffer = buf->buffer;

	if (!buffer)
		return;

	ring_buffer_record_disable(buffer);

	/* Make sure all commits have finished */
	synchronize_rcu();

	buf->time_start = buffer_ftrace_now(buf, buf->cpu);

	ring_buffer_reset_online_cpus(buffer);

	ring_buffer_record_enable(buffer);
}
/* …（同文件无关代码省略）… */
static void tracing_reset_all_cpus(struct array_buffer *buf)
{
	struct trace_buffer *buffer = buf->buffer;

	if (!buffer)
		return;

	ring_buffer_record_disable(buffer);

	/* Make sure all commits have finished */
	synchronize_rcu();

	buf->time_start = buffer_ftrace_now(buf, buf->cpu);

	ring_buffer_reset(buffer);

	ring_buffer_record_enable(buffer);
}
/* …（同文件无关代码省略）… */
int tracer_init(struct tracer *t, struct trace_array *tr)
{
	tracing_reset_online_cpus(&tr->array_buffer);
	update_last_data_if_empty(tr);
	return t->init(tr);
}
/* …（同文件无关代码省略）… */
struct trace_mod_entry {
	unsigned long	mod_addr;
	char		mod_name[MODULE_NAME_LEN];
};
/* …（同文件无关代码省略）… */
struct trace_scratch {
	unsigned int		clock_id;
	unsigned long		text_addr;
	unsigned long		nr_entries;
	struct trace_mod_entry	entries[];
};
/* …（同文件无关代码省略）… */
static int save_mod(struct module *mod, void *data)
{
	struct trace_array *tr = data;
	struct trace_scratch *tscratch;
	struct trace_mod_entry *entry;
	unsigned int size;

	tscratch = tr->scratch;
	if (!tscratch)
		return -1;
	size = tr->scratch_size;

	if (struct_size(tscratch, entries, tscratch->nr_entries + 1) > size)
		return -1;

	entry = &tscratch->entries[tscratch->nr_entries];

	tscratch->nr_entries++;

	entry->mod_addr = (unsigned long)mod->mem[MOD_TEXT].base;
	strscpy(entry->mod_name, mod->name);

	return 0;
}
/* …（同文件无关代码省略）… */
static void update_last_data(struct trace_array *tr)
{
	struct trace_module_delta *module_delta;
	struct trace_scratch *tscratch;

	if (!(tr->flags & TRACE_ARRAY_FL_BOOT))
		return;

	if (!(tr->flags & TRACE_ARRAY_FL_LAST_BOOT))
		return;

	/* Only if the buffer has previous boot data clear and update it. */
	tr->flags &= ~TRACE_ARRAY_FL_LAST_BOOT;

	/* If this is a backup instance, mark it for autoremove. */
	if (tr->flags & TRACE_ARRAY_FL_VMALLOC)
		tr->free_on_close = true;

	/* Reset the module list and reload them */
	if (tr->scratch) {
		struct trace_scratch *tscratch = tr->scratch;

		tscratch->clock_id = tr->clock_id;
		memset(tscratch->entries, 0,
		       flex_array_size(tscratch, entries, tscratch->nr_entries));
		tscratch->nr_entries = 0;

		guard(mutex)(&scratch_mutex);
		module_for_each_mod(save_mod, tr);
	}

	/*
	 * Need to clear all CPU buffers as there cannot be events
	 * from the previous boot mixed with events with this boot
	 * as that will cause a confusing trace. Need to clear all
	 * CPU buffers, even for those that may currently be offline.
	 */
	tracing_reset_all_cpus(&tr->array_buffer);

	/* Using current data now */
	tr->text_delta = 0;

	if (!tr->scratch)
		return;

	tscratch = tr->scratch;
	module_delta = READ_ONCE(tr->module_delta);
	WRITE_ONCE(tr->module_delta, NULL);
	kfree_rcu(module_delta, rcu);

	/* Set the persistent ring buffer meta data to this address */
	tscratch->text_addr = (unsigned long)_text;
}
/* …（同文件无关代码省略）… */
			return ret;
	}

	tr->current_trace_flags = t->flags ? : t->tracer->flags;

	if (trace->init) {
		ret = tracer_init(trace, tr);
/* …（同文件无关代码省略）… */
static bool update_last_data_if_empty(struct trace_array *tr)
{
	if (!(tr->flags & TRACE_ARRAY_FL_LAST_BOOT))
		return false;

	if (!ring_buffer_empty(tr->array_buffer.buffer))
		return false;

	/*
	 * If the buffer contains the last boot data and all per-cpu
	 * buffers are empty, reset it from the kernel side.
	 */
	update_last_data(tr);
	return true;
}
/* …（同文件无关代码省略）… */

	ret = ring_buffer_meta_seq_init(filp, tr->array_buffer.buffer, cpu);
	if (ret < 0)
		__trace_array_put(tr);
	return ret;
}

/* …（同文件无关代码省略）… */

static __init void tracer_init_tracefs_work_func(struct work_struct *work)
{

	event_trace_init();
