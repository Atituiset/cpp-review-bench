// AUTO-DRAFT from torvalds/linux PR #9a3415a159e7615a7043280ccc4ac310199df697
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
#define V3D_MAX_QUEUES (V3D_CPU + 1)
/* …（同文件无关代码省略）… */
struct v3d_stats {
	struct kref refcount;

	u64 start_ns;
	u64 enabled_ns;
	u64 jobs_completed;

	/*
	 * This seqcount is used to protect the access to the GPU stats
	 * variables. It must be used as, while we are reading the stats,
	 * IRQs can happen and the stats can be updated.
	 *
	 * However, we use the raw seqcount helpers to interact with this lock
	 * to avoid false positives from lockdep, which is unable to detect that
	 * our readers are never from irq or softirq context, and that, for CPU
	 * job queues, even the write side never is.
	 */
	seqcount_t lock;

	atomic_t reset_counter;
};
/* …（同文件无关代码省略）… */
struct v3d_queue_state {
	struct drm_gpu_scheduler sched;

	u64 fence_context;
	u64 emit_seqno;

	/* Stores the GPU stats for this queue in the global context. */
	struct v3d_stats *stats;

	/* Currently active job for this queue */
	struct v3d_job *active_job;
	spinlock_t queue_lock;
};
/* …（同文件无关代码省略）… */
struct v3d_perfmon {
	struct v3d_dev *v3d;

	/* Tracks the number of users of the perfmon, when this counter reaches
	 * zero the perfmon is destroyed.
	 */
	refcount_t refcnt;

	/* Number of counters activated in this perfmon instance
	 * (should be less than DRM_V3D_MAX_PERF_COUNTERS).
	 */
	u8 ncounters;

	/* Events counted by the HW perf counters. */
	u8 counters[DRM_V3D_MAX_PERF_COUNTERS];

	/* Storage for counter values. Counters are incremented by the
	 * HW perf counter values every time the perfmon is attached
	 * to a GPU job.  This way, perfmon users don't have to
	 * retrieve the results after each job if they want to track
	 * events covering several submissions.  Note that counter
	 * values can't be reset, but you can fake a reset by
	 * destroying the perfmon and creating a new one.
	 */
	u64 values[] __counted_by(ncounters);
};
/* …（同文件无关代码省略）… */
enum v3d_gen {
	V3D_GEN_33 = 33,
	V3D_GEN_41 = 41,
	V3D_GEN_42 = 42,
	V3D_GEN_71 = 71,
};
/* …（同文件无关代码省略）… */
struct v3d_dev {
	struct drm_device drm;

	/* Short representation (e.g. 33, 41) of the V3D tech version */
	enum v3d_gen ver;

	/* Short representation (e.g. 5, 6) of the V3D tech revision */
	int rev;

	bool single_irq_line;

	int irq[V3D_MAX_IRQS];

	struct v3d_perfmon_info perfmon_info;

	void __iomem *hub_regs;
	void __iomem *core_regs[3];
	void __iomem *bridge_regs;
	void __iomem *gca_regs;
	void __iomem *sms_regs;
	struct clk *clk;
	struct reset_control *reset;

	/* Virtual and DMA addresses of the single shared page table. */
	volatile u32 *pt;
	dma_addr_t pt_paddr;

	/* Virtual and DMA addresses of the MMU's scratch page.  When
	 * a read or write is invalid in the MMU, it will be
	 * redirected here.
	 */
	void *mmu_scratch;
	dma_addr_t mmu_scratch_paddr;
	/* virtual address bits from V3D to the MMU. */
	int va_width;

	/* Number of V3D cores. */
	u32 cores;

	/* Allocator managing the address space.  All units are in
	 * number of pages.
	 */
	struct drm_mm mm;
	spinlock_t mm_lock;

	struct work_struct overflow_mem_work;

	struct v3d_queue_state queue[V3D_MAX_QUEUES];

	/*
	 * Tracks the performance monitor state and consistency.
	 *
	 * When a non-global perfmon is attached to a job, the scheduler must
	 * not run any other job on the HW concurrently (otherwise, the
	 * counters would be polluted by unrelated work).
	 */
	struct {
		/* Protects @active. */
		spinlock_t lock;

		/* Perfmon currently programmed in HW (or NULL if none). */
		struct v3d_perfmon *active;

		/* Number of perfmons alive on this device. Jobs are not
		 * serialized if the number is zero.
		 */
		atomic_t nperfmons;

		/* Finished fence of the most recently submitted job that
		 * opened a serialization window (i.e. a job with a non-global
		 * perfmon attached).
		 */
		struct dma_fence *fence;

		/* Finished fence of the most recently submitted job on each HW
		 * queue, which is used so that a new perfmon-carrying job can
		 * depend on every job currently in-flight across all queues.
		 *
		 * Finished fences are only tracked if @nperfmons > 0 and no
		 * global perfmon is set.
		 */
		struct dma_fence *last_hw_fence[V3D_MAX_QUEUES];
	} perfmon_state;

	/* Protects bo_stats */
	struct mutex bo_lock;

	/* Ordered workqueue shared by every queue's scheduler timeout work.
	 * V3D reset is global to all queues, so the timeout handlers must not
	 * run concurrently.
	 */
	struct workqueue_struct *reset_wq;

	/* Lock taken when creating and pushing the GPU scheduler
	 * jobs, to keep the sched-fence seqnos in order.
	 */
	struct mutex sched_lock;

	/* Lock taken during a cache clean and when initiating an L2
	 * flush, to keep L2 flushes from interfering with the
	 * synchronous L2 cleans.
	 */
	struct mutex cache_clean_lock;

	struct {
		u32 num_allocated;
		u32 pages_allocated;
	} bo_stats;

	/* To support a performance analysis tool in user space, we require
	 * a single, globally configured performance monitor (perfmon) for
	 * all jobs.
	 */
	struct v3d_perfmon *global_perfmon;

	/* Global reset counter incremented on each GPU reset. */
	atomic_t reset_counter;
};
/* …（同文件无关代码省略）… */
struct v3d_file_priv {
	struct v3d_dev *v3d;

	struct xarray perfmons;

	struct drm_sched_entity sched_entity[V3D_MAX_QUEUES];

	/* Stores the GPU stats for a specific queue for this fd. */
	struct v3d_stats *stats[V3D_MAX_QUEUES];
};
/* …（同文件无关代码省略）… */
struct v3d_job {
	struct drm_sched_job base;

	struct kref refcount;

	struct v3d_dev *v3d;

	/* The queue that the job was submitted on. */
	enum v3d_queue queue;

	/* This is the array of BOs that were looked up at the start
	 * of submission.
	 */
	struct drm_gem_object **bo;
	u32 bo_count;

	/* v3d fence to be signaled by IRQ handler when the job is complete. */
	struct dma_fence *irq_fence;

	/* scheduler fence for when the job is considered complete and
	 * the BO reservations can be released.
	 */
	struct dma_fence *done_fence;

	/* Pointer to a performance monitor object if the user requested it,
	 * NULL otherwise.
	 */
	struct v3d_perfmon *perfmon;

	/* File descriptor of the process that submitted the job that could be used
	 * to collect per-process information about the GPU.
	 */
	struct v3d_file_priv *file_priv;

	/* Pointers to this job's per-fd and global queue stats. */
	struct v3d_stats *client_stats;
	struct v3d_stats *global_stats;

	/* Callback for the freeing of the job on refcount going to 0. */
	void (*free)(struct kref *ref);

	bool has_pm_ref;

	/* Whether the job needs implicit dependencies, i.e. must wait for
	 * other contexts still writing its BOs.
	 */
	bool has_implicit_dep;
};
/* …（同文件无关代码省略）… */
struct v3d_timestamp_query {
	/* Offset of this query in the timestamp BO for its value. */
	u32 offset;

	/* Syncobj that indicates the timestamp availability */
	struct drm_syncobj *syncobj;
};
/* …（同文件无关代码省略）… */
struct v3d_performance_query {
	/* Performance monitor IDs for this query */
	u32 *kperfmon_ids;

	/* Syncobj that indicates the query availability */
	struct drm_syncobj *syncobj;
};
/* …（同文件无关代码省略）… */
struct v3d_timestamp_query_info {
	struct v3d_timestamp_query *queries;

	u32 count;
};
/* …（同文件无关代码省略）… */
struct v3d_performance_query_info {
	struct v3d_performance_query *queries;

	/* Number of performance queries */
	u32 count;

	/* Number of performance monitors related to that query pool */
	u32 nperfmons;

	/* Number of performance counters related to that query pool */
	u32 ncounters;
};
/* …（同文件无关代码省略）… */
}

/* v3d_sched.c */
void v3d_timestamp_query_info_free(struct v3d_timestamp_query_info *query_info,
				   unsigned int count);
void v3d_performance_query_info_free(struct v3d_performance_query_info *query_info,
				     unsigned int count);
struct v3d_stats *v3d_stats_alloc(void);
void v3d_stats_release(struct kref *refcount);
void v3d_job_update_stats(struct v3d_job *job);
