// AUTO-DRAFT from torvalds/linux PR #f70886e5a510cd7fc2c3aef91c46a14f4485fe63
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#include <linux/nfs_mount.h>
#include <linux/nfs_page.h>
#include <linux/module.h>
#include <linux/file.h>
#include <linux/sched/mm.h>

/* …（同文件无关代码省略）… */
static void
ff_layout_remove_mirror(struct nfs4_ff_layout_mirror *mirror)
{
	struct inode *inode;
	if (mirror->layout == NULL)
		return;
	inode = mirror->layout->plh_inode;
	spin_lock(&inode->i_lock);
	list_del(&mirror->mirrors);
	spin_unlock(&inode->i_lock);
	mirror->layout = NULL;
}
/* …（同文件无关代码省略）… */
static void ff_layout_free_mirror(struct nfs4_ff_layout_mirror *mirror)
{
	const struct cred	*cred;
	u32 dss_id;

	ff_layout_remove_mirror(mirror);

	for (dss_id = 0; dss_id < mirror->dss_count; dss_id++) {
		kfree(mirror->dss[dss_id].fh_versions);
		cred = rcu_access_pointer(mirror->dss[dss_id].ro_cred);
		put_cred(cred);
		cred = rcu_access_pointer(mirror->dss[dss_id].rw_cred);
		put_cred(cred);
		nfs_close_local_fh(&mirror->dss[dss_id].nfl);
		/* the last reference to the mirror is gone; no concurrency */
		nfs4_ff_layout_put_deviceid(rcu_dereference_protected(
				mirror->dss[dss_id].mirror_ds, 1));
	}

	kvfree(mirror->dss);
	kfree(mirror);
}
/* …（同文件无关代码省略）… */
static void ff_layout_put_mirror(struct nfs4_ff_layout_mirror *mirror)
{
	if (mirror != NULL && refcount_dec_and_test(&mirror->ref))
		ff_layout_free_mirror(mirror);
}
/* …（同文件无关代码省略）… */
static void ff_layout_free_mirror_array(struct nfs4_ff_layout_segment *fls)
{
	u32 i;

	for (i = 0; i < fls->mirror_array_cnt; i++)
		ff_layout_put_mirror(fls->mirror_array[i]);
}
/* …（同文件无关代码省略）… */
{
	if (fls) {
		ff_layout_free_mirror_array(fls);
		kfree(fls);
	}
}
/* …（同文件无关代码省略）… */
static u32 ff_mirror_efficiency_sum(const struct nfs4_ff_layout_mirror *mirror)
{
	u32 dss_id, sum = 0;

	for (dss_id = 0; dss_id < mirror->dss_count; dss_id++)
		sum += mirror->dss[dss_id].efficiency;

	return sum;
}
/* …（同文件无关代码省略）… */
static void ff_layout_sort_mirrors(struct nfs4_ff_layout_segment *fls)
{
	int i, j;

	for (i = 0; i < fls->mirror_array_cnt - 1; i++) {
		for (j = i + 1; j < fls->mirror_array_cnt; j++)
			if (ff_mirror_efficiency_sum(fls->mirror_array[i]) <
			    ff_mirror_efficiency_sum(fls->mirror_array[j]))
				swap(fls->mirror_array[i],
				     fls->mirror_array[j]);
	}
}
/* …（同文件无关代码省略）… */
		fls->mirror_array[i]->report_interval = be32_to_cpup(p);

out_sort_mirrors:
	ff_layout_sort_mirrors(fls);
	ret = &fls->generic_hdr;
	dprintk("<-- %s (success)\n", __func__);
/* …（同文件无关代码省略）… */
static bool ff_layout_match_rw(const struct rpc_task *task,
			       const struct nfs_pgio_header *hdr,
			       const struct pnfs_layout_segment *lseg)
{
	return hdr->lseg == lseg;
}
/* …（同文件无关代码省略）… */
static bool ff_layout_match_commit(const struct rpc_task *task,
				   const struct nfs_commit_data *cdata,
				   const struct pnfs_layout_segment *lseg)
{
	return cdata->lseg == lseg;
}
/* …（同文件无关代码省略）… */
static bool ff_layout_match_io(const struct rpc_task *task, const void *data)
{
	const struct rpc_call_ops *ops = task->tk_ops;

	if (ops == &ff_layout_read_call_ops_v3 ||
	    ops == &ff_layout_read_call_ops_v4 ||
	    ops == &ff_layout_write_call_ops_v3 ||
	    ops == &ff_layout_write_call_ops_v4)
		return ff_layout_match_rw(task, task->tk_calldata, data);
	if (ops == &ff_layout_commit_call_ops_v3 ||
	    ops == &ff_layout_commit_call_ops_v4)
		return ff_layout_match_commit(task, task->tk_calldata, data);
	return false;
}
/* …（同文件无关代码省略）… */
			if (devid && memcmp(&mirror->dss[dss_id].devid, devid,
					    sizeof(*devid)) != 0)
				continue;
			rcu_read_lock();
			mirror_ds = rcu_dereference(mirror->dss[dss_id].mirror_ds);
			if (IS_ERR_OR_NULL(mirror_ds) ||
/* …（同文件无关代码省略）… */
			clnt = ds_clp->cl_rpcclient;
			if (!clnt)
				goto next;
			if (!rpc_cancel_tasks(clnt, -ECANCELED,
					      ff_layout_match_io, lseg))
				goto next;
			rpc_clnt_disconnect(clnt);
next:
			nfs4_ff_layout_put_deviceid(mirror_ds);
		}
