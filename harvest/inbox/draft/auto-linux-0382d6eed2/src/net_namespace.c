// AUTO-DRAFT from torvalds/linux PR #dd6f1e7582f0f7f12c2d620cf50446aa987d7327
/* should be called with nsid_lock held */
static int alloc_netid(struct net *net, struct net *peer, int reqid)
{
	int min = 0, max = 0;  // <<< BUG ANCHOR

	if (reqid >= 0) {
		min = reqid;
		max = reqid + 1;
	}

	return idr_alloc(&net->netns_ids, peer, min, max, GFP_ATOMIC);
}

/* This function is used by idr_for_each(). If net is equal to peer, the
/* …（同文件无关代码省略）… */
	 * use for_each_net_rcu() or net_rwsem.
	 */
	for_each_net(tmp) {
		int id = 0;

		spin_lock(&tmp->nsid_lock);
		while ((peer = idr_get_next(&tmp->netns_ids, &id))) {
			int curr_id = id;

			id++;
