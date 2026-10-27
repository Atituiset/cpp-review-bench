// AUTO-DRAFT from torvalds/linux PR #9e089e670e9ad1caf4a5b2b63e01b2dc076efbe3
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
};

struct nfs4_pnfs_ds {
	struct list_head	ds_node;  /* nfs4_pnfs_dev_hlist dev_dslist */
	char			*ds_remotestr;	/* comma sep list of addrs */
	struct list_head	ds_addrs;
	const struct net	*ds_net;
	struct nfs4_deviceid_node * (*alloc_deviceid_node)
			(struct nfs_server *server, struct pnfs_device *pdev,
			gfp_t gfp_flags);

	int (*prepare_layoutreturn) (struct nfs4_layoutreturn_args *);

	void (*cleanup_layoutcommit) (struct nfs4_layoutcommit_data *data);
	int (*prepare_layoutcommit) (struct nfs4_layoutcommit_args *args);
	int (*prepare_layoutstats) (struct nfs42_layoutstat_args *args);

	void (*cancel_io)(struct pnfs_layout_segment *lseg);
};

struct pnfs_commit_ops {
int pnfs_mark_matching_lsegs_return(struct pnfs_layout_hdr *lo,
				struct list_head *tmp_list,
				const struct pnfs_layout_range *recall_range,
				u32 seq, bool cancel_io);
int pnfs_mark_layout_stateid_invalid(struct pnfs_layout_hdr *lo,
		struct list_head *lseg_list);
bool pnfs_roc(struct inode *ino, struct nfs4_layoutreturn_args *args,
int pnfs_write_done_resend_to_mds(struct nfs_pgio_header *);
struct nfs4_threshold *pnfs_mdsthreshold_alloc(void);
void pnfs_error_mark_layout_for_return(struct inode *inode,
				       struct pnfs_layout_segment *lseg);
void pnfs_layout_return_unused_byclid(struct nfs_client *clp,
				      enum pnfs_iomode iomode);
int pnfs_layout_handle_reboot(struct nfs_client *clp);

/* nfs4_deviceid_flags */
	atomic_t			ref;
};

struct nfs4_deviceid_node *
nfs4_find_get_deviceid(struct nfs_server *server,
		const struct nfs4_deviceid *id, const struct cred *cred,
		gfp_t gfp_mask);
void nfs4_delete_deviceid(const struct pnfs_layoutdriver_type *, const struct nfs_client *, const struct nfs4_deviceid *);
void nfs4_init_deviceid_node(struct nfs4_deviceid_node *, struct nfs_server *,
			     const struct nfs4_deviceid *);
bool nfs4_put_deviceid_node(struct nfs4_deviceid_node *);
void nfs4_mark_deviceid_available(struct nfs4_deviceid_node *node);
void nfs4_mark_deviceid_unavailable(struct nfs4_deviceid_node *node);
bool nfs4_test_deviceid_unavailable(struct nfs4_deviceid_node *node);
void nfs4_deviceid_purge_client(const struct nfs_client *);
void nfs4_pnfs_v3_ds_connect_unload(void);
int nfs4_pnfs_ds_connect(struct nfs_server *mds_srv, struct nfs4_pnfs_ds *ds,
			  struct nfs4_deviceid_node *devid, unsigned int timeo,
			  unsigned int retrans, u32 version, u32 minor_version,
			  bool tightly_coupled);
struct nfs4_pnfs_ds_addr *nfs4_decode_mp_ds_addr(struct net *net,
						 struct xdr_stream *xdr,
						 gfp_t gfp_flags);
void pnfs_layout_mark_request_commit(struct nfs_page *req,
				     struct pnfs_layout_segment *lseg,
				     struct nfs_commit_info *cinfo,
}

static inline void pnfs_lseg_cancel_io(struct nfs_server *server,
				       struct pnfs_layout_segment *lseg)
{
	if (server->pnfs_curr_ld->cancel_io)
		server->pnfs_curr_ld->cancel_io(lseg);
}

extern unsigned int layoutstats_timer;
