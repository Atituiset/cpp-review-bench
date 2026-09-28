// AUTO-DRAFT from torvalds/linux PR #7cdf91542e1ed03aa8f5ab029702b3abde5be6d5
 * smaller shards if the LLC exceeds the target size. No-topo cids are packed
 * into their own max-sized shards.
 *
 * @core_cid: first cid of this cid's core (smt-sibling group)
 * @core_idx: global index of that core, in [0, nr_cores_at_init)
 * @llc_cid: first cid of this cid's LLC
