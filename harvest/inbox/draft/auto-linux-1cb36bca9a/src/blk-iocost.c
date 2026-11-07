// AUTO-DRAFT from torvalds/linux PR #c53d8fc9a7571b63482548a7e5284ef9f025fd3f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

enum ioc_running {
	IOC_IDLE,
	IOC_RUNNING,
	IOC_STOP,
};
/* …（同文件无关代码省略）… */
struct ioc_params {
	u32				qos[NR_QOS_PARAMS];
	u64				i_lcoefs[NR_I_LCOEFS];
	u64				lcoefs[NR_LCOEFS];
	u32				too_fast_vrate_pct;
	u32				too_slow_vrate_pct;
};
/* …（同文件无关代码省略）… */
struct ioc_margins {
	s64				min;
	s64				low;
	s64				target;
};
/* …（同文件无关代码省略）… */
struct ioc_missed {
	local_t				nr_met;
	local_t				nr_missed;
	u32				last_met;
	u32				last_missed;
};
/* …（同文件无关代码省略）… */
struct ioc_pcpu_stat {
	struct ioc_missed		missed[2];

	local64_t			rq_wait_ns;
	u64				last_rq_wait_ns;
};
/* …（同文件无关代码省略）… */
struct ioc {
	struct rq_qos			rqos;

	bool				enabled;

	struct ioc_params		params;
	struct ioc_margins		margins;
	u32				period_us;
	u32				timer_slack_ns;
	u64				vrate_min;
	u64				vrate_max;

	spinlock_t			lock;
	struct timer_list		timer;
	struct list_head		active_iocgs;	/* active cgroups */
	struct ioc_pcpu_stat __percpu	*pcpu_stat;

	enum ioc_running		running;
	atomic64_t			vtime_rate;
	u64				vtime_base_rate;
	s64				vtime_err;

	seqcount_spinlock_t		period_seqcount;
	u64				period_at;	/* wallclock starttime */
	u64				period_at_vtime; /* vtime starttime */

	atomic64_t			cur_period;	/* inc'd each period */
	int				busy_level;	/* saturation history */

	bool				weights_updated;
	atomic_t			hweight_gen;	/* for lazy hweights */

	/* debt forgivness */
	u64				dfgv_period_at;
	u64				dfgv_period_rem;
	u64				dfgv_usage_us_sum;

	u64				autop_too_fast_at;
	u64				autop_too_slow_at;
	int				autop_idx;
	bool				user_qos_params:1;
	bool				user_cost_model:1;
};
/* …（同文件无关代码省略）… */
	if (!ioc->enabled || !rq->alloc_time_ns || !rq->start_time_ns)
		return;

	switch (req_op(rq)) {
	case REQ_OP_READ:
		pidx = QOS_RLAT;
