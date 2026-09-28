// AUTO-DRAFT from torvalds/linux PR #7cdf91542e1ed03aa8f5ab029702b3abde5be6d5
#define SCX_CID_TOPO_NEG	(struct scx_cid_topo) {				\
	.core_cid = -1, .core_idx = -1, .llc_cid = -1, .llc_idx = -1,		\
	.node_cid = -1, .node_idx = -1, .shard_cid = -1, .shard_idx = -1,	\
}
/* …（同文件无关代码省略）… */
/**
 * scx_bpf_cid_topo - Copy out per-cid topology info
 * @cid: cid to look up
 * @out__uninit: where to copy the topology info; fully written by this call
 * @aux: implicit BPF argument to access bpf_prog_aux hidden from BPF progs
 *
 * Fill @out__uninit with the topology info for @cid. Trigger scx_error() if
 * @cid is out of range. If @cid is valid but in the no-topo section, all fields
 * are set to -1. All fields are also set to -1 when no cid tables have been
 * published yet, which a program may observe while racing the root enable.
 */
__bpf_kfunc void scx_bpf_cid_topo(s32 cid, struct scx_cid_topo *out__uninit,
				  const struct bpf_prog_aux *aux)
{  // <<< BUG ANCHOR
	struct scx_cid_topo *topo;
	struct scx_sched *sch;

	guard(rcu)();

	sch = scx_prog_sched(aux);
	topo = rcu_dereference(scx_cid_topo);
	if (unlikely(!sch) || !cid_valid(sch, cid) || unlikely(!topo)) {
		*out__uninit = SCX_CID_TOPO_NEG;
		return;
	}

	*out__uninit = topo[cid];
}

__bpf_kfunc_end_defs();
