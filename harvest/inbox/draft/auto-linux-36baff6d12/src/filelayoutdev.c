// AUTO-DRAFT from torvalds/linux PR #9e089e670e9ad1caf4a5b2b63e01b2dc076efbe3
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
void
nfs4_fl_free_deviceid(struct nfs4_file_layout_dsaddr *dsaddr)
{
	struct nfs4_pnfs_ds *ds;
	int i;

	nfs4_print_deviceid(&dsaddr->id_node.deviceid);

	for (i = 0; i < dsaddr->ds_num; i++) {
		ds = dsaddr->ds_list[i];
		if (ds != NULL)
			nfs4_pnfs_ds_put(ds);
	}
	kfree(dsaddr->stripe_indices);
	kfree_rcu(dsaddr, id_node.rcu);
}
/* …（同文件无关代码省略）… */
		trace_fl_getdevinfo(server, &pdev->dev_id, dsaddr->ds_list[i]->ds_remotestr);

		/* If DS was already in cache, free ds addrs */
		while (!list_empty(&dsaddrs)) {
			da = list_first_entry(&dsaddrs,
					      struct nfs4_pnfs_ds_addr,
					      da_node);
			list_del_init(&da->da_node);
			kfree(da->da_remotestr);
			kfree(da);
		}
	}

	folio_put(scratch);
	return dsaddr;

out_err_drain_dsaddrs:
	while (!list_empty(&dsaddrs)) {
		da = list_first_entry(&dsaddrs, struct nfs4_pnfs_ds_addr,
				      da_node);
		list_del_init(&da->da_node);
		kfree(da->da_remotestr);
		kfree(da);
	}
out_err_free_deviceid:
	nfs4_fl_free_deviceid(dsaddr);
	/* stripe_indicies was part of dsaddr */
/* …（同文件无关代码省略）… */
		goto out_test_devid;

	status = nfs4_pnfs_ds_connect(s, ds, devid, dataserver_timeo,
			     dataserver_retrans, 4,
			     s->nfs_client->cl_minorversion, true);
	if (status) {
		nfs4_mark_deviceid_unavailable(devid);
