// AUTO-DRAFT from torvalds/linux PR #59a02493f3b68fd0ad68c661393f680bcca5a85b
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <time.h>
  // <<< BUG ANCHOR
struct trace_array_cpu {
	local_t			disabled;

	unsigned long		entries;
	unsigned long		saved_latency;
	unsigned long		critical_start;
	unsigned long		critical_end;
/* …（同文件无关代码省略）… */
struct array_buffer {
	struct trace_array		*tr;
	struct trace_buffer		*buffer;
	struct trace_array_cpu __percpu	*data;
	u64				time_start;
	int				cpu;
};
/* …（同文件无关代码省略）… */
#define TRACE_FLAGS_MAX_SIZE		64
/* …（同文件无关代码省略）… */
struct trace_options {
	struct tracer			*tracer;
	struct trace_option_dentry	*topts;
	int				nr_topts;
};
/* …（同文件无关代码省略）… */
struct trace_func_repeats {
	unsigned long	ip;
	unsigned long	parent_ip;
	unsigned long	count;
	u64		ts_last_call;
};
/* …（同文件无关代码省略）… */
struct trace_module_delta {
	struct rcu_head	rcu;
	long		delta[];
};
/* …（同文件无关代码省略）… */
struct trace_array {
	struct list_head	list;
	char			*name;
	struct array_buffer	array_buffer;
#ifdef CONFIG_TRACER_SNAPSHOT
	/*
	 * The snapshot_buffer is used to snapshot the trace when a maximum
	 * latency is reached, or when the user initiates a snapshot.
	 * Some tracers will use this to store a maximum trace while
	 * it continues examining live traces.
	 *
	 * The buffers for the snapshot_buffer are set up the same as the
	 * array_buffer. When a snapshot is taken, the buffer of the
	 * snapshot_buffer is swapped with the buffer of the array_buffer
	 * and the buffers are reset for the array_buffer so the tracing can
	 * continue.
	 */
	struct array_buffer	snapshot_buffer;
	bool			allocated_snapshot;
	spinlock_t		snapshot_trigger_lock;
	unsigned int		snapshot;
#ifdef CONFIG_TRACER_MAX_TRACE
	unsigned long		max_latency;
	struct dentry		*d_max_latency;
#ifdef CONFIG_FSNOTIFY
	struct work_struct	fsnotify_work;
	struct irq_work		fsnotify_irqwork;
#endif /* CONFIG_FSNOTIFY */
#endif /* CONFIG_TRACER_MAX_TRACE */
#endif /* CONFIG_TRACER_SNAPSHOT */

	/* The below is for memory mapped ring buffer */
	unsigned int		mapped;
	unsigned long		range_addr_start;
	unsigned long		range_addr_size;
	char			*range_name;
	long			text_delta;
	struct trace_module_delta *module_delta;
	void			*scratch; /* pointer in persistent memory */
	int			scratch_size;

	int			buffer_disabled;

	struct trace_pid_list	__rcu *filtered_pids;
	struct trace_pid_list	__rcu *filtered_no_pids;
	/*
	 * max_lock is used to protect the swapping of buffers
	 * when taking a max snapshot. The buffers themselves are
	 * protected by per_cpu spinlocks. But the action of the swap
	 * needs its own lock.
	 *
	 * This is defined as a arch_spinlock_t in order to help
	 * with performance when lockdep debugging is enabled.
	 *
	 * It is also used in other places outside the update_max_tr
	 * so it needs to be defined outside of the
	 * CONFIG_TRACER_SNAPSHOT.
	 */
	arch_spinlock_t		max_lock;
#ifdef CONFIG_FTRACE_SYSCALLS
	int			sys_refcount_enter;
	int			sys_refcount_exit;
	struct trace_event_file	*enter_syscall_files[NR_syscalls];
	struct trace_event_file	*exit_syscall_files[NR_syscalls];
#endif
	int			stop_count;
	int			clock_id;
	int			nr_topts;
	bool			clear_trace;
	int			buffer_percent;
	unsigned int		n_err_log_entries;
	struct tracer		*current_trace;
	struct tracer_flags	*current_trace_flags;
	u64			trace_flags;
	unsigned char		trace_flags_index[TRACE_FLAGS_MAX_SIZE];
	unsigned int		flags;
	raw_spinlock_t		start_lock;
	union {
		const char	*system_names;
		char		*boot_events;
	};
	struct list_head	err_log;
	struct dentry		*dir;
	struct dentry		*options;
	struct dentry		*percpu_dir;
	struct eventfs_inode	*event_dir;
	struct trace_options	*topts;
	struct list_head	systems;
	struct list_head	events;
	struct list_head	marker_list;
	struct list_head	tracers;
	struct trace_event_file *trace_marker_file;
	cpumask_var_t		tracing_cpumask; /* only trace on set CPUs */
	/* one per_cpu trace_pipe can be opened by only one user */
	cpumask_var_t		pipe_cpumask;
	int			ref;
	int			trace_ref;
#ifdef CONFIG_MODULES
	struct list_head	mod_events;
#endif
#ifdef CONFIG_FUNCTION_TRACER
	struct ftrace_ops	*ops;
	struct trace_pid_list	__rcu *function_pids;
	struct trace_pid_list	__rcu *function_no_pids;
#ifdef CONFIG_FUNCTION_GRAPH_TRACER
	struct fgraph_ops	*gops;
#endif
#ifdef CONFIG_DYNAMIC_FTRACE
	/* All of these are protected by the ftrace_lock */
	struct list_head	func_probes;
	struct list_head	mod_trace;
	struct list_head	mod_notrace;
#endif
	/* function tracing enabled */
	int			function_enabled;
#endif
	int			no_filter_buffering_ref;
	unsigned int		syscall_buf_sz;
	struct list_head	hist_vars;
#ifdef CONFIG_TRACER_SNAPSHOT
	struct cond_snapshot	*cond_snapshot;
#endif
	struct trace_func_repeats	__percpu *last_func_repeats;
	/*
	 * On boot up, the ring buffer is set to the minimum size, so that
	 * we do not waste memory on systems that are not using tracing.
	 */
	bool ring_buffer_expanded;
	/*
	 * If the ring buffer is a read only backup instance, it will be
	 * removed after dumping all data via pipe, because no readable data.
	 */
	bool free_on_close;
	struct work_struct	autoremove_work;
};
/* …（同文件无关代码省略）… */
struct tracer_opt {
	const char	*name; /* Will appear on the trace_options file */
	u32		bit; /* Mask assigned in val field in tracer_flags */
};
/* …（同文件无关代码省略）… */
struct tracer_flags {
	u32			val;
	struct tracer_opt	*opts;
	struct tracer		*trace;
};
/* …（同文件无关代码省略）… */
struct trace_option_dentry {
	struct tracer_opt		*opt;
	struct tracer_flags		*flags;
	struct trace_array		*tr;
	struct dentry			*entry;
};
/* …（同文件无关代码省略）… */
struct tracer {
	const char		*name;
	int			(*init)(struct trace_array *tr);
	void			(*reset)(struct trace_array *tr);
	void			(*start)(struct trace_array *tr);
	void			(*stop)(struct trace_array *tr);
	int			(*update_thresh)(struct trace_array *tr);
	void			(*open)(struct trace_iterator *iter);
	void			(*pipe_open)(struct trace_iterator *iter);
	void			(*close)(struct trace_iterator *iter);
	void			(*pipe_close)(struct trace_iterator *iter);
	ssize_t			(*read)(struct trac
