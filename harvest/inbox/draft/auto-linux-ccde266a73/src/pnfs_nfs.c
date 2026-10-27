// AUTO-DRAFT from torvalds/linux PR #9e089e670e9ad1caf4a5b2b63e01b2dc076efbe3
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#include "nfs4session.h"
#include "internal.h"
#include "pnfs.h"
#include "netns.h"
#include "nfs4trace.h"
	struct nfs_commit_data *data = calldata;

	data->completion_ops->completion(data);
	pnfs_put_lseg(data->lseg);
	nfs_put_client(data->ds_clp);
	nfs_commitdata_release(data);
}

/*
 * Checks if 'dsaddrs1' contains a subset of 'dsaddrs2'. If it does,
 * declare a match.
 */
static bool
_same_data_server_addrs_locked(const struct list_head *dsaddrs1,
	struct sockaddr *sa1, *sa2;
	bool match = false;

	list_for_each_entry(da1, dsaddrs1, da_node) {
		sa1 = (struct sockaddr *)&da1->da_addr;
		match = false;
	return match;
}

/*
 * Lookup DS by addresses and NFS version.  nfs4_ds_cache_lock is held
 */
static struct nfs4_pnfs_ds *
_data_server_lookup_locked(const struct nfs_net *nn,
			   const struct list_head *dsaddrs, u32 version)
{
	struct nfs4_pnfs_ds *ds;

	list_for_each_entry(ds, &nn->nfs4_data_server_cache, ds_node)
		if (ds->ds_version == version &&
		    _same_data_server_addrs_locked(&ds->ds_addrs, dsaddrs))
			return ds;
	kfree(da);
}

static void destroy_ds(struct nfs4_pnfs_ds *ds)
{
	struct nfs4_pnfs_ds_addr *da;

	dprintk("--> %s\n", __func__);
	ifdebug(FACILITY)
		print_ds(ds);

	nfs_put_client(ds->ds_clp);

	while (!list_empty(&ds->ds_addrs)) {
		da = list_first_entry(&ds->ds_addrs,
				      struct nfs4_pnfs_ds_addr,
				      da_node);
		list_del_init(&da->da_node);
		nfs4_pnfs_ds_addr_free(da);
	}

	kfree(ds->ds_remotestr);
	kfree(ds);
	struct nfs_net *nn = net_generic(ds->ds_net, nfs_net_id);

	if (refcount_dec_and_lock(&ds->ds_count, &nn->nfs4_data_server_lock)) {
		list_del_init(&ds->ds_node);
		spin_unlock(&nn->nfs4_data_server_lock);
		destroy_ds(ds);
	}
{
	struct nfs_net *nn = net_generic(net, nfs_net_id);
	struct nfs4_pnfs_ds *tmp_ds, *ds = NULL;
	char *remotestr;

	if (list_empty(dsaddrs)) {

	/* this is only used for debugging, so it's ok if its N
