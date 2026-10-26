// AUTO-DRAFT from torvalds/linux PR #9e089e670e9ad1caf4a5b2b63e01b2dc076efbe3
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR

#include "nfs4session.h"
#include "internal.h"
#include "pnfs.h"
#include "netns.h"
#include "nfs4trace.h"
/* …（同文件无关代码省略）… */
	struct nfs_commit_data *data = calldata;

	data->completion_ops->completion(data);
	pnfs_put_lseg(data->lseg);
	nfs_put_client(data->ds_clp);
	nfs_commitdata_release(data);
/* …（同文件无关代码省略）… */
static void
print_ds(struct nfs4_pnfs_ds *ds)
{
	if (ds == NULL) {
		printk(KERN_WARNING "%s NULL device\n", __func__);
		return;
	}
	printk(KERN_WARNING "        ds %s\n"
		"        ref count %d\n"
		"        client %p\n"
		"        cl_exchange_flags %x\n",
		ds->ds_remotestr,
		refcount_read(&ds->ds_count), ds->ds_clp,
		ds->ds_clp ? ds->ds_clp->cl_exchange_flags : 0);
}
/* …（同文件无关代码省略）… */
static bool
same_sockaddr(struct sockaddr *addr1, struct sockaddr *addr2)
{
	struct sockaddr_in *a, *b;
	struct sockaddr_in6 *a6, *b6;

	if (addr1->sa_family != addr2->sa_family)
		return false;

	switch (addr1->sa_family) {
	case AF_INET:
		a = (struct sockaddr_in *)addr1;
		b = (struct sockaddr_in *)addr2;

		if (a->sin_addr.s_addr == b->sin_addr.s_addr &&
		    a->sin_port == b->sin_port)
			return true;
		break;

	case AF_INET6:
		a6 = (struct sockaddr_in6 *)addr1;
		b6 = (struct sockaddr_in6 *)addr2;

		/* LINKLOCAL addresses must have matching scope_id */
		if (ipv6_addr_src_scope(&a6->sin6_addr) ==
		    IPV6_ADDR_SCOPE_LINKLOCAL &&
		    a6->sin6_scope_id != b6->sin6_scope_id)
			return false;

		if (ipv6_addr_equal(&a6->sin6_addr, &b6->sin6_addr) &&
		    a6->sin6_port == b6->sin6_port)
			return true;
		break;

	default:
		dprintk("%s: unhandled address family: %u\n",
			__func__, addr1->sa_family);
		return false;
	}

	return false;
}

/*
 * Checks if 'dsaddrs1' contains a subset of 'dsaddrs2'. If it does,
 * declare a match.
 */
static bool
_same_data_server_addrs_locked(const struct list_head *dsaddrs1,
			       const struct list_head *dsaddrs2)
{
	struct nfs4_pnfs_ds_addr *da1, *da2;
	struct sockaddr *sa1, *sa2;
	bool match = false;

	list_for_each_entry(da1, dsaddrs1, da_node) {
		sa1 = (struct sockaddr *)&da1->da_addr;
		match = false;
		list_for_each_entry(da2, dsaddrs2, da_node) {
			sa2 = (struct sockaddr *)&da2->da_addr;
			match = same_sockaddr(sa1, sa2);
			if (match)
				break;
		}
		if (!match)
			break;
	}
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
/* …（同文件无关代码省略）… */
static void nfs4_pnfs_ds_addr_free(struct nfs4_pnfs_ds_addr *da)
{
	kfree(da->da_remotestr);
	kfree(da->da_netid);
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
/* …（同文件无关代码省略）… */
	struct nfs_net *nn = net_generic(ds->ds_net, nfs_net_id);

	if (refcount_dec_and_lock(&ds->ds_count, &nn->nfs4_data_server_lock)) {
		list_del_init(&ds->ds_node);
		spin_unlock(&nn->nfs4_data_server_lock);
		destroy_ds(ds);
	}
/* …（同文件无关代码省略）… */
static char *
nfs4_pnfs_remotestr(struct list_head *dsaddrs, gfp_t gfp_flags)
{
	struct nfs4_pnfs_ds_addr *da;
	char *remotestr;
	size_t len;
	char *p;

	len = 3;        /* '{', '}' and eol */
	list_for_each_entry(da, dsaddrs, da_node) {
		len += strlen(da->da_remotestr) + 1;    /* string plus comma */
	}

	remotestr = kzalloc(len, gfp_flags);
	if (!remotestr)
		return NULL;

	p = remotestr;
	*(p++) = '{';
	len--;
	list_for_each_entry(da, dsaddrs, da_node) {
		size_t ll = strlen(da->da_remotestr);

		if (ll > len)
			goto out_err;

		memcpy(p, da->da_remotestr, ll);
		p += ll;
		len -= ll;

		if (len < 1)
			goto out_err;
		(*p++) = ',';
		len--;
	}
	if (len < 2)
		goto out_err;
	*(p++) = '}';
	*p = '\0';
	return remotestr;
out_err:
	kfree(remotestr);
	return NULL;
}
/* …（同文件无关代码省略）… */
{
	struct nfs_net *nn = net_generic(net, nfs_net_id);
	struct nfs4_pnfs_ds *tmp_ds, *ds = NULL;
	char *remotestr;

	if (list_empty(dsaddrs)) {
/* …（同文件无关代码省略）… */

	/* this is only used for debugging, so it's ok if its NULL */
	remotestr = nfs4_pnfs_remotestr(dsaddrs, gfp_flags);

	spin_lock(&nn->nfs4_data_server_lock);
	tmp_ds = _data_server_lookup_locked(nn, dsaddrs, version);
/* …（同文件无关代码省略）… */
		list_splice_init(dsaddrs, &ds->ds_addrs);
		ds->ds_remotestr = remotestr;
		refcount_set(&ds->ds_count, 1);
		INIT_LIST_HEAD(&ds->ds_node);
		ds->ds_net = net;
		ds->ds_clp = NULL;
		ds->ds_version = version;
		list_add(&ds->ds_node, &nn->nfs4_data_server_cache);
		dprintk("%s add new data server %s\n", __func__,
			ds->ds_remotestr);
	} else {
/* …（同文件无关代码省略）… */
			int ds_addrlen,
			int ds_proto,
			unsigned int ds_timeo,
			unsigned int ds_retrans);

static bool load_v3_ds_connect(void)
{
/* …（同文件无关代码省略）… */
static int _nfs4_pnfs_v3_ds_connect(struct nfs_server *mds_srv,
				 struct nfs4_pnfs_ds *ds,
				 unsigned int timeo,
				 unsigned int retrans)
{
	struct nfs_client *clp = ERR_PTR(-EIO);
	struct nfs_client *mds_clp = mds_srv->nfs_client;
/* …（同文件无关代码省略）… */
			ds_proto = XPRT_TRANSPORT_TCP_TLS;

		clp = get_v3_ds_connect(mds_srv, &da->da_addr, da->da_addrlen,
					ds_proto, timeo, retrans);
		if (IS_ERR(clp))
			continue;
		clp->cl_rpcclient->cl_softerr = 0;
/* …（同文件无关代码省略）… */
static int _nfs4_pnfs_v4_ds_connect(struct nfs_server *mds_srv,
				 struct nfs4_pnfs_ds *ds,
				 unsigned int timeo,
