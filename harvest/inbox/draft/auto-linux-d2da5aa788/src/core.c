// AUTO-DRAFT from torvalds/linux PR #a27611f8994c9a4772156432badb533af33fd32d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
static inline u64 perf_event_clock(struct perf_event *event)
{
	return event->clock();
}
/* …（同文件无关代码省略）… */
static inline bool
perf_cgroup_match(struct perf_event *event)
{
	struct perf_cpu_context *cpuctx = this_cpu_ptr(&perf_cpu_context);

	/* @event doesn't care about cgroup */
	if (!event->cgrp)
		return true;

	/* wants specific cgroup scope but @cpuctx isn't associated with any */
	if (!cpuctx->cgrp)
		return false;

	/*
	 * Cgroup scoping is recursive.  An event enabled for a cgroup is
	 * also enabled for all its descendant cgroups.  If @cpuctx's
	 * cgroup is a descendant of @event's (the test covers identity
	 * case), it's a match.
	 */
	return cgroup_is_descendant(cpuctx->cgrp->css.cgroup,
				    event->cgrp->css.cgroup);
}
/* …（同文件无关代码省略）… */
static u32 perf_event_pid_type(struct perf_event *event, struct task_struct *p,
				enum pid_type type)
{
	u32 nr;
	/*
	 * only top level events have the pid namespace they were created in
	 */
	if (event->parent)
		event = event->parent;

	nr = __task_pid_nr_ns(p, type, event->ns);
	/* avoid -1 if it is idle thread or runs in another ns */
	if (!nr && !pid_alive(p))
		nr = -1;
	return nr;
}
/* …（同文件无关代码省略）… */
static u32 perf_event_pid(struct perf_event *event, struct task_struct *p)
{
	return perf_event_pid_type(event, p, PIDTYPE_TGID);
}
/* …（同文件无关代码省略）… */
static u32 perf_event_tid(struct perf_event *event, struct task_struct *p)
{
	return perf_event_pid_type(event, p, PIDTYPE_PID);
}
/* …（同文件无关代码省略）… */
static u64 primary_event_id(struct perf_event *event)
{
	u64 id = event->id;

	if (event->parent)
		id = event->parent->id;

	return id;
}
/* …（同文件无关代码省略）… */
static inline int
event_filter_match(struct perf_event *event)
{
	return (event->cpu == -1 || event->cpu == smp_processor_id()) &&
	       perf_cgroup_match(event);
}
/* …（同文件无关代码省略）… */
{
	/*
	 * Both perf_pending_task() and perf_pending_irq() can race with the
	 * task exiting.
	 */
	if (current->flags & PF_EXITING)
		return;

	/*
/* …（同文件无关代码省略）… */
#define PERF_SAMPLE_ID_ALL  (PERF_SAMPLE_TID | PERF_SAMPLE_TIME |	\
			     PERF_SAMPLE_ID | PERF_SAMPLE_STREAM_ID |	\
			     PERF_SAMPLE_CPU | PERF_SAMPLE_IDENTIFIER)
/* …（同文件无关代码省略）… */
static void __perf_event_header__init_id(struct perf_sample_data *data,
					 struct perf_event *event,
					 u64 sample_type)
{
	data->type = event->attr.sample_type;
	data->sample_flags |= data->type & PERF_SAMPLE_ID_ALL;

	if (sample_type & PERF_SAMPLE_TID) {
		/* namespace issues */
		data->tid_entry.pid = perf_event_pid(event, current);
		data->tid_entry.tid = perf_event_tid(event, current);
	}

	if (sample_type & PERF_SAMPLE_TIME)
		data->time = perf_event_clock(event);

	if (sample_type & (PERF_SAMPLE_ID | PERF_SAMPLE_IDENTIFIER))
		data->id = primary_event_id(event);

	if (sample_type & PERF_SAMPLE_STREAM_ID)
		data->stream_id = event->id;

	if (sample_type & PERF_SAMPLE_CPU) {
		data->cpu_entry.cpu	 = raw_smp_processor_id();
		data->cpu_entry.reserved = 0;
	}
}

void perf_event_header__init_id(struct perf_event_header *header,
				struct perf_sample_data *data,
				struct perf_event *event)
{
	if (event->attr.sample_id_all) {
		header->size += event->id_header_size;
		__perf_event_header__init_id(data, event, event->attr.sample_type);
/* …（同文件无关代码省略）… */
static void __perf_event__output_id_sample(struct perf_output_handle *handle,
					   struct perf_sample_data *data)
{
	u64 sample_type = data->type;

	if (sample_type & PERF_SAMPLE_TID)
		perf_output_put(handle, data->tid_entry);

	if (sample_type & PERF_SAMPLE_TIME)
		perf_output_put(handle, data->time);

	if (sample_type & PERF_SAMPLE_ID)
		perf_output_put(handle, data->id);

	if (sample_type & PERF_SAMPLE_STREAM_ID)
		perf_output_put(handle, data->stream_id);

	if (sample_type & PERF_SAMPLE_CPU)
		perf_output_put(handle, data->cpu_entry);

	if (sample_type & PERF_SAMPLE_IDENTIFIER)
		perf_output_put(handle, data->id);
}
/* …（同文件无关代码省略）… */
void perf_event__output_id_sample(struct perf_event *event,
				  struct perf_output_handle *handle,
				  struct perf_sample_data *sample)
{
	if (event->attr.sample_id_all)
		__perf_event__output_id_sample(handle, sample);
}
/* …（同文件无关代码省略）… */
struct perf_read_event {
	struct perf_event_header	header;

	u32				pid;
	u32				tid;
};
/* …（同文件无关代码省略）… */
	struct perf_output_handle handle;
	struct perf_sample_data sample;
	struct perf_read_event read_event = {
		.header = {
			.type = PERF_RECORD_READ,
			.misc = 0,
			.size = sizeof(read_event) + event->read_size,
		},
		.pid = perf_event_pid(event, task),
		.tid = perf_event_tid(event, task),
	};
	int ret;

	perf_event_header__init_id(&read_event.header, &sample, event);
	ret = perf_output_begin(&handle, &sample, event, read_event.header.size);
	if (ret)
		return;
/* …（同文件无关代码省略）… */
static void
perf_iterate_ctx(struct perf_event_context *ctx,
		   perf_iterate_f output,
		   void *data, bool all)
{
	struct perf_event *event;

	list_for_each_entry_rcu(event, &ctx->event_list, event_entry) {
		if (!all) {
			if (event->state < PERF_EVENT_STATE_INACTIVE)
				continue;
			if (!event_filter_match(event))
				continue;
		}

		output(event, data);
	}
}
/* …（同文件无关代码省略）… */
static void perf_iterate_sb_cpu(perf_iterate_f output, void *data)
{
	struct pmu_event_list *pel = this_cpu_ptr(&pmu_sb_events);
	struct perf_event *event;

	list_for_each_entry_rcu(event, &pel->list, sb_list) {
		/*
		 * Skip events that are not fully formed yet; ensure that
		 * if we observe event->ctx, both event and ctx will be
		 * complete enough. See perf_install_in_context().
		 */
		if (!smp_load_acquire(&event->ctx))
			continue;

		if (event->state < PERF_EVENT_STATE_INACTIVE)
			continue;
		if (!event_filter_match(event))
			continue;
		output(event, data);
	}
}
/* …（同文件无关代码省略）… */
static void
perf_iterate_sb(perf_iterate_f output, void *data,
	       struct perf_event_context *task_ctx)
{
	struct perf_event_context *ctx;

	rcu_read_lock();
	preempt_disable();

	/*
	 * If we have task_ctx != NULL we only notify the task context itself.
	 * The task_ctx is set only for EXIT events before releasing task
	 * context.
	 */
	if (task_ctx) {
		perf_iterate_ctx(task_ctx, output, data, false);
		goto done;
	}

	perf_iterate_sb_cpu(output, data);

	ctx = rcu_dereference(current->perf_event_ctxp);
	if (ctx)
		perf_iterate_ctx(ctx, output, data, false);
done:
	preempt_enable();
	rcu_read_unlock();
}
/* …（同文件无关代码省略）… */
		u32				ptid;
		u64				time;
	} event_id;
};

static int perf_event_task_match(struct perf_event *event)
{
	return event->attr.comm  || event->attr.mmap ||
	       event->attr.mmap2 || event->attr.mmap_data ||
	       event->attr.task;
}
/* …（同文件无关代码省略）… */
static void perf_event_task_output(struct perf_event *event,
				   void *data)
{
	struct perf_task_event *task_event = data;
	struct perf_output_handle handle;
	struct perf_sample_data	sample;
	struct task_struct *task = task_event->task;
	int ret, size = task_event->event_id.header.size;

	if (!perf_event_task_match(event))
		return;

	perf_event_header__init_id(&task_event->event_id.header, &sample, event);

	ret = perf_output_begin(&handle, &sample, event,
				task_event->event_id.header.size);
	if (ret)
		goto out;

	task_event->event_id.pid = perf_event_pid(event, task);
	task_event->event_id.tid = perf_event_tid(event, task);

	if (task_event->event_id.header.type == PERF_RECORD_EXIT) {
		task_event->event_id.ppid = perf_event_pid(event,
							task->real_parent);
		task_event->event_id.ptid = perf_event_pid(event,
							task->real_parent);
	} else {  /* PERF_RECORD_FORK */
		task_event->event_id.ppid = perf_event_pid(event, current);
		task_event->event_id.ptid = perf_event_tid(event, current);
	}

	task_event->event_id.time = perf_event_clock(event);

	perf_output_put(&handle, task_event->event_id);

	perf_event__output_id_sample(event, &handle, &sample);

	perf_output_end(&handle);
out:
	task_event->event_id.header.size = size;
}

static void perf_event_task(struct task_struct *task,
/* …（同文件无关代码省略）… */
	task_event = (struct perf_task_event){
		.task	  = task,
		.task_ctx = task_ctx,
		.event_id    = {
			.header = {
				.type = new ? PERF_RECORD_FORK : PERF_RECORD_EXIT,
				.misc = 0,
				.size = sizeof(task_event.event_id),
			},
			/* .pid  */
			/* .ppid */
			/* .tid  */
			/* .ptid */
			/* .time */
		},
	};

	perf_iterate_sb(perf_event_task_output,
/* …（同文件无关代码省略）… */
struct perf_comm_event {
	struct task_struct	*task;
	char			*comm;
	int			comm_size;

	struct {
		struct perf_event_header	header;

		u32				pid;
		u32				tid;
	} event_id;
};

static int perf_event_comm_match(struct perf_event *event)
{
	return event->attr.comm;
}
/* …（同文件无关代码省略）… */
static void perf_event_comm_output(struct perf_event *event,
				   void *data)
{
	struct perf_comm_event *comm_event = data;
	struct perf_output_handle handle;
	struct perf_sample_data sample;
	int size = comm_event->event_id.header.size;
	int ret;

	if (!perf_event_comm_match(event))
		return;

	perf_event_header__init_id(&comm_event->event_id.header, &sample, event);
	ret = perf_output_begin(&handle, &sample, event,
				comm_event->event_id.header.size);

	if (ret)
		goto out;

	comm_event->event_id.pid = perf_event_pid(event, comm_event->task);
	comm_event->event_id.tid = perf_event_tid(event, comm_event->task);

	perf_output_put(&handle, comm_event->event_id);
	__output_copy(&handle, comm_event->comm,
				   comm_event->comm_size);

	perf_event__output_id_sample(event, &handle, &sample);

	perf_output_end(&handle);
out:
	comm_event->event_id.header.size = size;
}

static void perf_event_comm_event(struct perf_comm_event *comm_event)
{
	char comm[TASK_COMM_LEN];
	unsigned int size;

	memset(comm, 0, sizeof(comm));
	strscpy(comm, comm_event->task->comm);
	size = ALIGN(strlen(comm)+1, sizeof(u64));

	comm_event->comm = comm;
	comm_event->comm_size = size;

	comm_event->event_id.header.size = sizeof(comm_event->event_id) + size;

	perf_iterate_sb(perf_event_comm_output,
		       comm_event,
		       NULL);
}
/* …（同文件无关代码省略）… */
		.task	= task,
		/* .comm      */
		/* .comm_size */
		.event_id  = {
			.header = {
				.type = PERF_RECORD_COMM,
				.misc = exec ? PERF_RECORD_MISC_COMM_EXEC : 0,
				/* .size */
			},
			/* .pid */
			/* .tid */
		},
	};

	perf_event_comm_event(&comm_event);
/* …（同文件无关代码省略）… */
static int perf_event_namespaces_match(struct perf_event *event)
{
	return event->attr.namespaces;
}
/* …（同文件无关代码省略）… */
	struct perf_namespaces_event *namespaces_event = data;
	struct perf_output_handle handle;
	struct perf_sample_data sample;
	u16 header_size = namespaces_event->event_id.header.size;
	int ret;

	if (!perf_event_namespaces_match(event))
		return;

	perf_event_header__init_id(&namespaces_event->event_id.header,
				   &sample, event);
	ret = perf_output_begin(&handle, &sample, event,
				namespaces_event->event_id.header.size);
	if (ret)
		goto out;

	namespaces_event->event_id.pid = perf_event_pid(event,
							namespaces_event->task);
/* …（同文件无关代码省略）… */
	perf_event__output_id_sample(event, &handle, &sample);

	perf_output_end(&handle);
out:
	namespaces_event->event_id.header.size = header_size;
}

static void perf_fill_ns_link_info(struct perf_ns_link_info *ns_link_info,
/* …（同文件无关代码省略）… */
	namespaces_event = (struct perf_namespaces_event){
		.task	= task,
		.event_id  = {
			.header = {
				.type = PERF_RECORD_NAMESPACES,
				.misc = 0,
				.size = sizeof(namespaces_event.event_id),
			},
			/* .pid */
			/* .tid */
			.nr_namespaces = NR_NAMESPACES,
/* …（同文件无关代码省略）… */
static int perf_event_cgroup_match(struct perf_event *event)
{
	return event->attr.cgroup;
}
/* …（同文件无关代码省略）… */
static void perf_event_cgroup_output(struct perf_event *event, void *data)
{
	struct perf_cgroup_event *cgroup_event = data;
	struct perf_output_handle handle;
	struct perf_sample_data sample;
	u16 header_size = cgroup_event->event_id.header.size;
	int ret;

	if (!perf_event_cgroup_match(event))
		return;

	perf_event_header__init_id(&cgroup_event->event_id.header,
				   &sample, event);
	ret = perf_output_begin(&handle, &sample, event,
				cgroup_event->event_id.header.size);
	if (ret)
		goto out;

	perf_output_put(&handle, cgroup_event->event_id);
	__output_copy(&handle, cgroup_event->path, cgroup_event->path_size);

	perf_event__output_id_sample(event, &handle, &sample);

	perf_output_end(&handle);
out:
	cgroup_event->event_id.header.size = header_size;
}

static void perf_event_cgroup(struct cgroup *cgrp)
/* …（同文件无关代码省略）… */

	cgroup_event = (struct perf_cgroup_event){
		.event_id  = {
			.header = {
				.type = PERF_RECORD_CGROUP,
				.misc = 0,
				.size = sizeof(cgroup_event.event_id),
			},
			.id = cgroup_id(cgrp),
		},
	};
/* …（同文件无关代码省略）… */
	while (!IS_ALIGNED(size, sizeof(u64)))
		cgroup_event.path[size++] = '\0';

	cgroup_event.event_id.header.size += size;
	cgroup_event.path_size = size;

	perf_iterate_sb(perf_event_cgroup_output,
/* …（同文件无关代码省略）… */
struct perf_mmap_event {
	struct vm_area_struct	*vma;

	const char		*file_name;
	int			file_size;
	int			maj, min;
	u64			ino;
	u64			ino_generation;
	u32			prot, flags;
	u8			build_id[BUILD_ID_SIZE_MAX];
	u32			build_id_size;

	struct {
		struct perf_event_header	header;

		u32				pid;
		u32				tid;
		u64				start;
		u64				len;
		u64				pgoff;
	} event_id;
};
/* …（同文件无关代码省略）… */
static int perf_event_mmap_match(struct perf_event *event,
				 void *data)
{
	struct perf_mmap_event *mmap_event = data;
	struct vm_area_struct *vma = mmap_event->vma;
	int executable = vma->vm_flags & VM_EXEC;

	return (!executable && event->attr.mmap_data) ||
	       (executable && (event->attr.mmap || event->attr.mmap2));
}
/* …（同文件无关代码省略）… */
static void perf_event_mmap_output(struct perf_event *event,
				   void *data)
{
	struct perf_mmap_event *mmap_event = data;
	struct perf_output_handle handle;
	struct perf_sample_data sample;
	int size = mmap_event->event_id.header.size;
	u32 type = mmap_event->event_id.header.type;
	bool use_build_id;
	int ret;

	if (!perf_event_mmap_match(event, data))
		return;

	if (event->attr.mmap2) {
		mmap_event->event_id.header.type = PERF_RECORD_MMAP2;
		mmap_event->event_id.header.size += sizeof(mmap_event->maj);
		mmap_event->event_id.header.size += sizeof(mmap_event->min);
		mmap_event->event_id.header.size += sizeof(mmap_event->ino);
		mmap_event->event_id.header.size += sizeof(mmap_event->ino_generation);
		mmap_event->event_id.header.size += sizeof(mmap_event->prot);
		mmap_event->event_id.header.size += sizeof(mmap_event->flags);
	}

	perf_event_header__init_id(&mmap_event->event_id.header, &sample, event);
	ret = perf_output_begin(&handle, &sample, event,
				mmap_event->event_id.header.size);
	if (ret)
		goto out;

	mmap_event->event_id.pid = perf_event_pid(event, current);
	mmap_event->event_id.tid = perf_event_tid(event, current);

	use_build_id = event->attr.build_id && mmap_event->build_id_size;

	if (event->attr.mmap2 && use_build_id)
		mmap_event->event_id.header.misc |= PERF_RECORD_MISC_MMAP_BUILD_ID;

	perf_output_put(&handle, mmap_event->event_id);

	if (event->attr.mmap2) {
		if (use_build_id) {
			u8 size[4] = { (u8) mmap_event->build_id_size, 0, 0, 0 };

			__output_copy(&handle, size, 4);
			__output_copy(&handle, mmap_event->build_id, BUILD_ID_SIZE_MAX);
		} else {
			perf_output_put(&handle, mmap_event->maj);
			perf_output_put(&handle, mmap_event->min);
			perf_output_put(&handle, mmap_event->ino);
			perf_output_put(&handle, mmap_event->ino_generation);
		}
		perf_output_put(&handle, mmap_event->prot);
		perf_output_put(&handle, mmap_event->flags);
	}

	__output_copy(&handle, mmap_event->file_name,
				   mmap_event->file_size);

	perf_event__output_id_sample(event, &handle, &sample);

	perf_output_end(&handle);
out:
	mmap_event->event_id.header.size = size;
	mmap_event->event_id.header.type = type;
}

static void perf_event_mmap_event(struct perf_mmap_event *mmap_event)
{
	struct vm_area_struct *vma = mmap_event->vma;
	struct file *file = vma->vm_file;
	int maj = 0, min = 0;
	u64 ino = 0, gen = 0;
	u32 prot = 0, flags = 0;
	unsigned int size;
	char tmp[16];
	char *buf = NULL;
	char *name = NULL;

	if (vma->vm_flags & VM_READ)
		prot |= PROT_READ;
	if (vma->vm_flags & VM_WRITE)
		prot |= PROT_WRITE;
	if (vma->vm_flags & VM_EXEC)
		prot |= PROT_EXEC;

	if (vma->vm_flags & VM_MAYSHARE)
		flags = MAP_SHARED;
	else
		flags = MAP_PRIVATE;

	if (vma->vm_flags & VM_LOCKED)
		flags |= MAP_LOCKED;
	if (is_vm_hugetlb_page(vma))
		flags |= MAP_HUGETLB;

	if (file) {
		const struct inode *inode;
		dev_t dev;

		buf = kmalloc(PATH_MAX, GFP_KERNEL);
		if (!buf) {
			name = "//enomem";
			goto cpy_name;
		}
		/*
		 * d_path() works from the end of the rb backwards, so we
		 * need to add enough zero bytes after the string to handle
		 * the 64bit alignment we do later.
		 */
		name = d_path(file_user_path(file), buf, PATH_MAX - sizeof(u64));
		if (IS_ERR(name)) {
			name = "//toolong";
			goto cpy_name;
		}
		inode = file_user_inode(vma->vm_file);
		dev = inode->i_sb->s_dev;
		ino = inode->i_ino;
		gen = inode->i_generation;
		maj = MAJOR(dev);
		min = MINOR(dev);

		goto got_name;
	} else {
		if (vma->vm_ops && vma->vm_ops->name)
			name = (char *) vma->vm_ops->name(vma);
		if (!name)
			name = (char *)arch_vma_name(vma);
		if (!name) {
			if (vma_is_initial_heap(vma))
				name = "[heap]";
			else if (vma_is_initial_stack(vma))
				name = "[stack]";
			else
				name = "//anon";
		}
	}

cpy_name:
	strscpy(tmp, name);
	name = tmp;
got_name:
	/*
	 * Since our buffer works in 8 byte units we need to align our string
	 * size to a multiple of 8. However, we must guarantee the tail end is
	 * zero'd out to avoid leaking random bits to userspace.
	 */
	size = strlen(name)+1;
	while (!IS_ALIGNED(size, sizeof(u64)))
		name[size++] = '\0';

	mmap_event->file_name = name;
	mmap_event->file_size = size;
	mmap_event->maj = maj;
	mmap_event->min = min;
	mmap_event->ino = ino;
	mmap_event->ino_generation = gen;
	mmap_event->prot = prot;
	mmap_event->flags = flags;

	if (!(vma->vm_flags & VM_EXEC))
		mmap_event->event_id.header.misc |= PERF_RECORD_MISC_MMAP_DATA;

	mmap_event->event_id.header.size = sizeof(mmap_event->event_id) + size;

	if (atomic_read(&nr_build_id_events))
		build_id_parse_nofault(vma, mmap_event->build_id, &mmap_event->build_id_size);

	perf_iterate_sb(perf_event_mmap_output,
		       mmap_event,
		       NULL);

	kfree(buf);
}
/* …（同文件无关代码省略）… */
		/* .file_name */
		/* .file_size */
		.event_id  = {
			.header = {
				.type = PERF_RECORD_MMAP,
				.misc = PERF_RECORD_MISC_USER,
				/* .size */
			},
			/* .pid */
			/* .tid */
			.start  = vma->vm_start,
/* …（同文件无关代码省略）… */
		u64				size;
		u64				flags;
	} rec = {
		.header = {
			.type = PERF_RECORD_AUX,
			.misc = 0,
			.size = sizeof(rec),
		},
		.offset		= head,
		.size		= size,
		.flags		= flags,
	};
	int ret;

	perf_event_header__init_id(&rec.header, &sample, event);
	ret = perf_output_begin(&handle, &sample, event, rec.header.size);

	if (ret)
/* …（同文件无关代码省略）… */
		struct perf_event_header	header;
		u64				lost;
	} lost_samples_event = {
		.header = {
			.type = PERF_RECORD_LOST_SAMPLES,
			.misc = 0,
			.size = sizeof(lost_samples_event),
		},
		.lost		= lost,
	};

	perf_event_header__init_id(&lost_samples_event.header, &sample, event);

	ret = perf_output_begin(&handle, &sample, event,
				lost_samples_event.header.size);
/* …（同文件无关代码省略）… */
		u32				next_prev_pid;
		u32				next_prev_tid;
	} event_id;
};

static int perf_event_switch_match(struct perf_event *event)
{
	return event->attr.context_switch;
}
/* …（同文件无关代码省略）… */
static void perf_event_switch_output(struct perf_event *event, void *data)
{
	struct perf_switch_event *se = data;
	struct perf_output_handle handle;
	struct perf_sample_data sample;
	int ret;

	if (!perf_event_switch_match(event))
		return;

	/* Only CPU-wide events are allowed to see next/prev pid/tid */
	if (event->ctx->task) {
		se->event_id.header.type = PERF_RECORD_SWITCH;
		se->event_id.header.size = sizeof(se->event_id.header);
	} else {
		se->event_id.header.type = PERF_RECORD_SWITCH_CPU_WIDE;
		se->event_id.header.size = sizeof(se->event_id);
		se->event_id.next_prev_pid =
					perf_event_pid(event, se->next_prev);
		se->event_id.next_prev_tid =
					perf_event_tid(event, se->next_prev);
	}

	perf_event_header__init_id(&se->event_id.header, &sample, event);

	ret = perf_output_begin(&handle, &sample, event, se->event_id.header.size);
	if (ret)
		return;

	if (event->ctx->task)
		perf_output_put(&handle, se->event_id.header);
	else
		perf_output_put(&handle, se->event_id);

	perf_event__output_id_sample(event, &handle, &sample);

	perf_output_end(&handle);
}
/* …（同文件无关代码省略）… */
	switch_event = (struct perf_switch_event){
		.task		= task,
		.next_prev	= next_prev,
		.event_id	= {
			.header = {
				/* .type */
				.misc = sched_in ? 0 : PERF_RECORD_MISC_SWITCH_OUT,
				/* .size */
			},
			/* .next_prev_pid */
			/* .next_prev_tid */
		},
	};

	if (!sched_in && task_is_runnable(task)) {
		switch_event.event_id.header.misc |=
				PERF_RECORD_MISC_SWITCH_OUT_PREEMPT;
	}

	perf_iterate_sb(perf_event_switch_output, &switch_event, NULL);
}

/* …（同文件无关代码省略）… */
		u64				id;
		u64				stream_id;
	} throttle_event = {
		.header = {
			.type = PERF_RECORD_THROTTLE,
			.misc = 0,
			.size = sizeof(throttle_event),
		},
		.time		= perf_event_clock(event),
		.id		= primary_event_id(event),
		.stream_id	= event->id,
	};

	if (enable)
		throttle_event.header.type = PERF_RECORD_UNTHROTTLE;

	perf_event_header__init_id(&throttle_event.header, &sample, event);

	ret = perf_output_begin(&handle, &sample, event,
				throttle_event.header.size);
/* …（同文件无关代码省略）… */
static int perf_event_ksymbol_match(struct perf_event *event)
{
	return event->attr.ksymbol;
}
/* …（同文件无关代码省略）… */
	struct perf_output_handle handle;
	struct perf_sample_data sample;
	int ret;

	if (!perf_event_ksymbol_match(event))
		return;

	perf_event_header__init_id(&ksymbol_event->event_id.header,
				   &sample, event);
	ret = perf_output_begin(&handle, &sample, event,
				ksymbol_event->event_id.header.size);
	if (ret)
/* …（同文件无关代码省略）… */
		.name = name,
		.name_len = name_len,
		.event_id = {
			.header = {
				.type = PERF_RECORD_KSYMBOL,
				.size = sizeof(ksymbol_event.event_id) +
					name_len,
			},
			.addr = addr,
			.len = len,
			.ksym_type = ksym_type,
/* …（同文件无关代码省略）… */
static int perf_event_bpf_match(struct perf_event *event)
{
	return event->attr.bpf_event;
}
/* …（同文件无关代码省略）… */
	if (!perf_event_bpf_match(event))
		return;

	perf_event_header__init_id(&bpf_event->event_id.header,
				   &sample, event);
	ret = perf_output_begin(&handle, &sample, event,
				bpf_event->event_id.header.size);
	if (ret)
/* …（同文件无关代码省略）… */
	bpf_event = (struct perf_bpf_event){
		.prog = prog,
		.event_id = {
			.header = {
				.type = PERF_RECORD_BPF_EVENT,
				.size = sizeof(bpf_event.event_id),
			},
			.type = type,
			.flags = flags,
			.id = prog->aux->id,
/* …（同文件无关代码省略）… */
struct perf_callchain_deferred_event {
	struct unwind_stacktrace *trace;
	struct {
		struct perf_event_header	header;
		u64				cookie;
		u64				nr;
		u64				ips[];
	} event;
};
/* …（同文件无关代码省略）… */
static void perf_callchain_deferred_output(struct perf_event *event, void *data)
{
	struct perf_callchain_deferred_event *deferred_event = data;
	struct perf_output_handle handle;
	struct perf_sample_data sample;
	int ret, size = deferred_event->event.header.size;

	if (!event->attr.defer_output)
		return;

	/* XXX do we really need sample_id_all for this ??? */
	perf_event_header__init_id(&deferred_event->event.header, &sample, event);

	ret = perf_output_begin(&handle, &sample, event,
				deferred_event->event.header.size);
	if (ret)
		goto out;

	perf_output_put(&handle, deferred_event->event);
	for (int i = 0; i < deferred_event->trace->nr; i++) {
		u64 entry = deferred_event->trace->entries[i];
		perf_output_put(&handle, entry);
	}
	perf_event__output_id_sample(event, &handle, &sample);

	perf_output_end(&handle);
out:
	deferred_event->event.header.size = size;
}

static void perf_unwind_deferred_callback(struct unwind_work *work,
					 struct unwind_stacktrace *trace, u64 cookie)
{
	struct perf_callchain_deferred_event deferred_event = {
		.trace = trace,
		.event = {
			.header = {
				.type = PERF_RECORD_CALLCHAIN_DEFERRED,
				.misc = PERF_RECORD_MISC_USER,
				.size = sizeof(deferred_event.event) +
					(trace->nr * sizeof(u64)),
			},
			.cookie = cookie,
			.nr = trace->nr,
		},
	};

	perf_iterate_sb(perf_callchain_deferred_output, &deferred_event, NULL);
}
/* …（同文件无关代码省略）… */
struct perf_text_poke_event {
	const void		*old_bytes;
	const void		*new_bytes;
	size_t			pad;
	u16			old_len;
	u16			new_len;

/* …（同文件无关代码省略）… */
static int perf_event_text_poke_match(struct perf_event *event)
{
	return event->attr.text_poke;
}
/* …（同文件无关代码省略）… */
	struct perf_text_poke_event *text_poke_event = data;
	struct perf_output_handle handle;
	struct perf_sample_data sample;
	u64 padding = 0;
	int ret;

	if (!perf_event_text_poke_match(event))
		return;

	perf_event_header__init_id(&text_poke_event->event_id.header, &sample, event);

	ret = perf_output_begin(&handle, &sample, event,
				text_poke_event->event_id.header.size);
/* …（同文件无关代码省略）… */
	text_poke_event = (struct perf_text_poke_event){
		.old_bytes    = old_bytes,
		.new_bytes    = new_bytes,
		.pad          = pad,
		.old_len      = old_len,
		.new_len      = new_len,
		.event_id  = {
			.header = {
				.type = PERF_RECORD_TEXT_POKE,
				.misc = PERF_RECORD_MISC_KERNEL,
				.size = sizeof(text_poke_event.event_id) + tot + pad,
			},
			.addr = (unsigned long)addr,
		},
	};
/* …（同文件无关代码省略）… */
	    event->attach_state & PERF_ATTACH_ITRACE)
		return;

	rec.header.type	= PERF_RECORD_ITRACE_START;
	rec.header.misc	= 0;
	rec.header.size	= sizeof(rec);
	rec.pid	= perf_event_pid(event, current);
	rec.tid	= perf_event_tid(event, current);

	perf_event_header__init_id(&rec.header, &sample, event);
	ret = perf_output_begin(&handle, &sample, event, rec.header.size);

	if (ret)
/* …（同文件无关代码省略）… */
	if (event->parent)
		event = event->parent;

	rec.header.type	= PERF_RECORD_AUX_OUTPUT_HW_ID;
	rec.header.misc	= 0;
	rec.header.size	= sizeof(rec);
	rec.hw_id	= hw_id;

	perf_event_header__init_id(&rec.header, &sample, event);
	ret = perf_output_begin(&handle, &sample, event, rec.header.size);

	if (ret)
/* …（同文件无关代码省略）… */
	if (err)
		return err;

	if (!attr.exclude_kernel ||
	    ((attr.sample_type & PERF_SAMPLE_CALLCHAIN) &&
	     !attr.exclude_callchain_kernel)) {
		err = perf_allow_kernel();
/* …（同文件无关代码省略）… */
int perf_allow_kernel(void)
{
	if (sysctl_perf_event_paranoid > 1 && !perfmon_capable())
		return -EACCES;

	return security_perf_event_open(PERF_SECURITY_KERNEL);
}
