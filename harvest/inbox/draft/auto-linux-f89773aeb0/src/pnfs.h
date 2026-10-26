// AUTO-DRAFT from torvalds/linux PR #9e089e670e9ad1caf4a5b2b63e01b2dc076efbe3
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
struct nfs4_pnfs_ds_addr {
	struct sockaddr_storage	da_addr;
	size_t			da_addrlen;
	struct list_head	da_node;  /* nfs4_pnfs_dev_hlist dev_dslist */
	char			*da_remotestr;	/* human readable addr+port */
	const char		*da_netid;
	int			da_transport;
};

struct nfs4_pnfs_ds {
	struct list_head	ds_node;  /* nfs4_pnfs_dev_hlist dev_dslist */
	char			*ds_remotestr;	/* comma sep list of addrs */
	struct list_head	ds_addrs;
	const struct net	*ds_net;
/* …（同文件无关代码省略）… */
struct pnfs_layout_segment {
	struct list_head pls_list;
	struct list_head pls_lc_list;
	struct list_head pls_commits;
	struct pnfs_layout_range pls_range;
	refcount_t pls_refcount;
	u32 pls_seq;
	unsigned long pls_flags;
	struct pnfs_layout_hdr *pls_layout;
};
/* …（同文件无关代码省略）… */
enum pnfs_try_status {
	PNFS_ATTEMPTED     = 0,
	PNFS_NOT_ATTEMPTED = 1,
	PNFS_TRY_AGAIN     = 2,
};
/* …（同文件无关代码省略）… */
struct pnfs_layoutdriver_type {
	struct list_head pnfs_tblid;
	const u32 id;
	const char *name;
	struct module *owner;
	unsigned flags;
	unsigned max_layoutget_response;

	int (*set_layoutdriver) (struct nfs_server *, const struct nfs_fh *);
	int (*clear_layoutdriver) (struct nfs_server *);

	struct pnfs_layout_hdr * (*alloc_layout_hdr) (struct inode *inode, gfp_t gfp_flags);
	void (*free_layout_hdr) (struct pnfs_layout_hdr *);

	struct pnfs_layout_segment * (*alloc_lseg) (struct pnfs_layout_hdr *layoutid, struct nfs4_layoutget_res *lgr, gfp_t gfp_flags);
	void (*free_lseg) (struct pnfs_layout_segment *lseg);
	void (*add_lseg) (struct pnfs_layout_hdr *layoutid,
			struct pnfs_layout_segment *lseg,
			struct list_head *free_me);

	void (*return_range) (struct pnfs_layout_hdr *lo,
			      struct pnfs_layout_range *range);

	/* test for nfs page cache coalescing */
	const struct nfs_pageio_ops *pg_read_ops;
	const struct nfs_pageio_ops *pg_write_ops;

	struct pnfs_ds_commit_info *(*get_ds_info) (struct inode *inode);

	int (*sync)(struct inode *inode, bool datasync);

	/*
	 * Return PNFS_ATTEMPTED to indicate the layout code has attempted
	 * I/O, else return PNFS_NOT_ATTEMPTED to fall back to normal NFS
	 */
	enum pnfs_try_status (*read_pagelist)(struct nfs_pgio_header *);
	enum pnfs_try_status (*write_pagelist)(struct nfs_pgio_header *, int);

	void (*free_deviceid_node) (struct nfs4_deviceid_node *);
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
/* …（同文件无关代码省略）… */
struct pnfs_layout_hdr {
	refcount_t		plh_refcount;
	atomic_t		plh_outstanding; /* number of RPCs out */
	struct list_head	plh_layouts;   /* other client layouts */
	struct list_head	plh_bulk_destroy;
	struct list_head	plh_segs;      /* layout segments list */
	struct list_head	plh_return_segs; /* invalid layout segments */
	unsigned long		plh_block_lgets; /* block LAYOUTGET if >0 */
	unsigned long		plh_retry_timestamp;
	unsigned long		plh_flags;
	nfs4_stateid		plh_stateid;
	u32			plh_barrier; /* ignore lower seqids */
	u32			plh_return_seq;
	enum pnfs_iomode	plh_return_iomode;
	loff_t			plh_lwb; /* last write byte for layoutcommit */
	const struct cred	*plh_lc_cred; /* layoutcommit cred */
	struct inode		*plh_inode;
	struct rcu_head		plh_rcu;
};
/* …（同文件无关代码省略）… */
struct pnfs_device {
	struct nfs4_deviceid dev_id;
	unsigned int  layout_type;
	unsigned int  mincount;
	unsigned int  maxcount;	/* gdia_maxcount */
	struct page **pages;
	unsigned int  pgbase;
	unsigned int  pglen;	/* reply buffer length */
	unsigned char nocache : 1;/* May not be cached */
};
/* …（同文件无关代码省略）… */
int pnfs_mark_matching_lsegs_return(struct pnfs_layout_hdr *lo,
				struct list_head *tmp_list,
				const struct pnfs_layout_range *recall_range,
				u32 seq, bool cancel_io);
int pnfs_mark_layout_stateid_invalid(struct pnfs_layout_hdr *lo,
		struct list_head *lseg_list);
bool pnfs_roc(struct inode *ino, struct nfs4_layoutreturn_args *args,
/* …（同文件无关代码省略）… */
int pnfs_write_done_resend_to_mds(struct nfs_pgio_header *);
struct nfs4_threshold *pnfs_mdsthreshold_alloc(void);
void pnfs_error_mark_layout_for_return(struct inode *inode,
				       struct pnfs_layout_segment *lseg);
void pnfs_layout_return_unused_byclid(struct nfs_client *clp,
				      enum pnfs_iomode iomode);
int pnfs_layout_handle_reboot(struct nfs_client *clp);

/* nfs4_deviceid_flags */
/* …（同文件无关代码省略）… */
struct nfs4_deviceid_node {
	struct hlist_node		node;
	struct hlist_node		tmpnode;
	const struct pnfs_layoutdriver_type *ld;
	const struct nfs_client		*nfs_client;
	unsigned long 			flags;
	unsigned long			timestamp_unavailable;
	struct nfs4_deviceid		deviceid;
	struct rcu_head			rcu;
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
/* …（同文件无关代码省略）… */
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
/* …（同文件无关代码省略）… */
}

static inline void pnfs_lseg_cancel_io(struct nfs_server *server,
				       struct pnfs_layout_segment *lseg)
{
	if (server->pnfs_curr_ld->cancel_io)
		server->pnfs_curr_ld->cancel_io(lseg);
}

extern unsigned int layoutstats_timer;
/* …（同文件无关代码省略）… */
static inline int pnfs_layout_handle_reboot(struct nfs_client *clp)
{
	return 0;
}
/* …（同文件无关代码省略）… */
static inline bool pnfs_roc(struct inode *ino,
			    struct nfs4_layoutreturn_args *args,
			    struct nfs4_layoutreturn_res *res,
			    const struct cred *cred, bool sync)
{
	return false;
}
/* …（同文件无关代码省略）… */
static inline struct nfs4_threshold *pnfs_mdsthreshold_alloc(void)
{
	return NULL;
}
/* …（同文件无关代码省略）… */
static inline void nfs4_pnfs_v3_ds_connect_unload(void)
{
}
