// AUTO-DRAFT from torvalds/linux PR #e4fb7e5e8e7e9edc167c3057802e440b77dcb856
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
struct trace_eprobe {
	/* tracepoint system */
	const char *event_system;

	/* tracepoint event */
	const char *event_name;

	/* filter string for the tracepoint */
	char *filter_str;

	struct trace_event_call *event;

	struct dyn_event	devent;
	struct trace_probe	tp;
};
/* …（同文件无关代码省略）… */
struct eprobe_data {
	struct trace_event_file	*file;
	struct trace_eprobe	*ep;
};
/* …（同文件无关代码省略）… */
static struct event_trigger_data *
new_eprobe_trigger(struct trace_eprobe *ep, struct trace_event_file *file)
{
	struct event_trigger_data *trigger;
	struct event_filter *filter = NULL;
	struct eprobe_data *edata;
	int ret;

	edata = kzalloc_obj(*edata);
	trigger = kzalloc_obj(*trigger);
	if (!trigger || !edata) {
		ret = -ENOMEM;
		goto error;
	}

	trigger->flags = EVENT_TRIGGER_FL_PROBE;
	trigger->count = -1;

	/*
	 * EVENT PROBE triggers are not registered as commands with
	 * register_event_command(), as they are not controlled by the user
	 * from the trigger file
	 */
	trigger->cmd_ops = &event_trigger_cmd;

	INIT_LIST_HEAD(&trigger->list);

	if (ep->filter_str) {
		ret = create_event_filter(file->tr, ep->event,
					ep->filter_str, false, &filter);
		if (ret)
			goto error;
	}
	RCU_INIT_POINTER(trigger->filter, filter);

	edata->file = file;
	edata->ep = ep;
	trigger->private_data = edata;

	return trigger;
error:
	free_event_filter(filter);
	kfree(edata);
	kfree(trigger);
	return ERR_PTR(ret);
}
/* …（同文件无关代码省略）… */
static int enable_eprobe(struct trace_eprobe *ep,
			 struct trace_event_file *eprobe_file)
{
	struct event_trigger_data *trigger;
	struct trace_event_file *file;
	struct trace_array *tr = eprobe_file->tr;

	file = find_event_file(tr, ep->event_system, ep->event_name);
	if (!file)
		return -ENOENT;
	trigger = new_eprobe_trigger(ep, eprobe_file);
	if (IS_ERR(trigger))
		return PTR_ERR(trigger);

	list_add_tail_rcu(&trigger->list, &file->triggers);

	trace_event_trigger_enable_disable(file, 1);
	update_cond_flag(file);

	return 0;
}
/* …（同文件无关代码省略）… */
static int disable_eprobe(struct trace_eprobe *ep,
			  struct trace_array *tr)
{
	struct event_trigger_data *trigger = NULL, *iter;
	struct trace_event_file *file;
	struct event_filter *filter;
	struct eprobe_data *edata;

	file = find_event_file(tr, ep->event_system, ep->event_name);
	if (!file)
		return -ENOENT;

	list_for_each_entry(iter, &file->triggers, list) {
		if (!(iter->flags & EVENT_TRIGGER_FL_PROBE))
			continue;
		edata = iter->private_data;
		if (edata->ep == ep) {
			trigger = iter;
			break;
		}
	}
	if (!trigger)
		return -ENODEV;

	list_del_rcu(&trigger->list);

	trace_event_trigger_enable_disable(file, 0);
	update_cond_flag(file);

	/* Make sure nothing is using the edata or trigger */
	tracepoint_synchronize_unregister();

	filter = rcu_access_pointer(trigger->filter);

	if (filter)
		free_event_filter(filter);
	kfree(edata);
	kfree(trigger);

	return 0;
}
/* …（同文件无关代码省略）… */
{
	struct trace_probe *tp;
	struct trace_eprobe *ep;
	bool enabled;
	int ret = 0;
	int cnt = 0;

	tp = trace_probe_primary_from_call(call);
	if (WARN_ON_ONCE(!tp))
		return -ENODEV;
	enabled = trace_probe_is_enabled(tp);

	/* This also changes "enabled" state */
	if (file) {
		ret = trace_probe_add_file(tp, file);
		if (ret)
			return ret;
	} else
		trace_probe_set_flag(tp, TP_FLAG_PROFILE);

	if (enabled)
		return 0;

	for_each_trace_eprobe_tp(ep, tp) {
		ret = enable_eprobe(ep, file);
		if (ret)
			break;
		enabled = true;
		cnt++;
	}

	if (ret) {
		/* Failed to enable one of them. Roll back all */
		if (enabled) {
			/*
			 * It's a bug if one failed for something other than memory
			 * not being available but another eprobe succeeded.
/* …（同文件无关代码省略）… */
					break;
			}
		}
		if (file)
			trace_probe_remove_file(tp, file);
		else
			trace_probe_clear_flag(tp, TP_FLAG_PROFILE);
	}

	return ret;
/* …（同文件无关代码省略）… */
	struct trace_probe *tp;
	struct trace_eprobe *ep;

	tp = trace_probe_primary_from_call(call);
	if (WARN_ON_ONCE(!tp))
		return -ENODEV;

	if (file) {
		if (!trace_probe_get_file_link(tp, file))
			return -ENOENT;
		if (!trace_probe_has_single_file(tp))
			goto out;
		trace_probe_clear_flag(tp, TP_FLAG_TRACE);
	} else
		trace_probe_clear_flag(tp, TP_FLAG_PROFILE);

	if (!trace_probe_is_enabled(tp)) {
		for_each_trace_eprobe_tp(ep, tp)
			disable_eprobe(ep, file->tr);
	}

 out:
	if (file)
		/*
		 * Synchronization is done in below function. For perf event,
		 * file == NULL and perf_trace_event_unreg() calls
		 * tracepoint_synchronize_unregister() to ensure synchronize
		 * event. We don't need to care about it.
		 */
		trace_probe_remove_file(tp, file);

	return 0;
}
