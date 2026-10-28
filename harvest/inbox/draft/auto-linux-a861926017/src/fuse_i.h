// AUTO-DRAFT from torvalds/linux PR #03973f7f3556b769a3dd5104f166614d446351c9
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
# define pr_fmt(fmt) "fuse: " fmt
#endif

#include "args.h"
#include <linux/fuse.h>
#include <linux/fs.h>
/* …（同文件无关代码省略）… */
#include <linux/backing-dev.h>
#include <linux/mutex.h>
#include <linux/rwsem.h>
#include <linux/rbtree.h>
#include <linux/poll.h>
#include <linux/workqueue.h>
#include <linux/kref.h>
#include <linux/xattr.h>
#include <linux/pid_namespace.h>
#include <linux/refcount.h>
#include <linux/user_namespace.h>

/** Default max number of pages that can be used in a single read request */
#define FUSE_DEFAULT_MAX_PAGES_PER_REQ 32
/* …（同文件无关代码省略）… */
struct fuse_submount_lookup {
	/** @count: Refcount */
	refcount_t count;

	/**
	 * @nodeid: Unique ID, which identifies the inode between userspace
	 * and kernel
	 */
	u64 nodeid;

	/** @forget: The request used for sending the FORGET message */
	struct fuse_forget_link *forget;
};

/* Container for data related to mapping to backing file */
struct fuse_backing {
	struct file *file;
	const struct cred *cred;

	/* refcount */
	refcount_t count;
	struct rcu_head rcu;
/* …（同文件无关代码省略）… */
struct fuse_inode {
	/** @inode: Inode data */
	struct inode inode;

	/**
	 * @nodeid: Unique ID, which identifies the inode between userspace
	 * and kernel
	 */
	u64 nodeid;

	/** @nlookup: Number of lookups on this inode */
	u64 nlookup;

	/** @forget: The request used for sending the FORGET message */
	struct fuse_forget_link *forget;

	/** @i_time: Time in jiffies until the file attributes are valid */
	u64 i_time;

	/** @inval_mask: Which attributes are invalid */
	u32 inval_mask;

	/**
	 * @orig_i_mode: The sticky bit in inode->i_mode may have been removed,
	 * so preserve the original mode
	 */
	umode_t orig_i_mode;

	/** @i_btime: Cache birthtime */
	struct timespec64 i_btime;

	/** @orig_ino: 64-bit inode number */
	u64 orig_ino;

	/** @attr_version: Version of last attribute change */
	u64 attr_version;

	union {
		/* read/write io cache (regular file only) */
		struct {
			/**
			 * @write_files: Files usable in writepage.
			 * Protected by fi->lock
			 */
			struct list_head write_files;

			/**
			 * @queued_writes: Writepages pending on truncate or
			 * fsync
			 */
			struct list_head queued_writes;

			/**
			 * @writectr: Number of sent writes, a negative bias
			 * (FUSE_NOWRITE) means more writes are blocked
			 */
			int writectr;

			/** @iocachectr: Number of files/maps using page cache */
			int iocachectr;

			/** @page_waitq: Waitq for writepage completion */
			wait_queue_head_t page_waitq;

			/** @direct_io_waitq: waitq for direct-io completion */
			wait_queue_head_t direct_io_waitq;
		};

		/** @rdc: readdir cache (directory only) */
		struct {
			/** @cached: true if fully cached */
			bool cached;

			/** @size: size of cache */
			loff_t size;

			/**
			 * @pos: position at end of cache (position of next
			 * entry)
			 */
			loff_t pos;

			/** @version: version of the cache */
			u64 version;

			/**
			 * @mtime: modification time of directory when cache was
			 * started
			 */
			struct timespec64 mtime;

			/**
			 * @epoch: epoch of fc when cache was started
			 */
			int epoch;

			/**
			 * @iversion: iversion of directory when cache was
			 * started
			 */
			u64 iversion;

			/** @lock: protects above fields */
			spinlock_t lock;
		} rdc;
	};

	/** @state: Miscellaneous bits describing inode state */
	unsigned long state;

	/**
	 * @mutex: Lock for serializing lookup and readdir for back
	 * compatibility
	 */
	struct mutex mutex;

	/** @lock: Lock to protect write-related fields */
	spinlock_t lock;

#ifdef CONFIG_FUSE_DAX
	/**
	 * @dax: Dax specific inode data
	 */
	struct fuse_inode_dax *dax;
#endif
	/** @submount_lookup: Submount specific lookup tracking */
	struct fuse_submount_lookup *submount_lookup;
#ifdef CONFIG_FUSE_PASSTHROUGH
	/** @fb: Reference to backing file in passthrough mode */
	struct fuse_backing *fb;
#endif

	/**
	 * @cached_i_blkbits: The underlying inode->i_blkbits value will not
	 * be modified, so preserve the blocksize specified by the server.
	 */
	u8 cached_i_blkbits;
};
/* …（同文件无关代码省略）… */
struct fuse_file {
	/** @fm: Fuse connection for this file */
	struct fuse_mount *fm;

	/** @args: Argument space reserved for open/release */
	union fuse_file_args *args;

	/** @kh: Kernel file handle guaranteed to be unique */
	u64 kh;

	/** @fh: File handle used by userspace */
	u64 fh;

	/** @nodeid: Node id of this file */
	u64 nodeid;

	/** @count: Refcount */
	refcount_t count;

	/** @open_flags: FOPEN_* flags returned by open */
	u32 open_flags;

	/** @write_entry: Entry on inode's write_files list */
	struct list_head write_entry;

	/** @readdir: Readdir-related */
	struct {
		/** @pos: Dir stream position */
		loff_t pos;

		/** @cache_off: Offset in cache */
		loff_t cache_off;

		/** @version: Version of cache we are reading */
		u64 version;

	} readdir;

	/** @polled_node: RB node to be linked on fuse_conn->polled_files */
	struct rb_node polled_node;

	/** @poll_wait: Wait queue head for poll */
	wait_queue_head_t poll_wait;

	/** @iomode: Does file hold a fi->iocachectr refcount? */
	enum { IOM_NONE, IOM_CACHED, IOM_UNCACHED } iomode;

#ifdef CONFIG_FUSE_PASSTHROUGH
	/** @passthrough: Reference to backing file in passthrough mode */
	struct file *passthrough;
	/** @cred: passthrough file credentials */
	const struct cred *cred;
#endif

	/** @flock: Has flock been performed on this file? */
	bool flock:1;
};
/* …（同文件无关代码省略）… */
struct fuse_release_args {
	struct fuse_args args;
	struct fuse_release_in inarg;
	struct inode *inode;
};
/* …（同文件无关代码省略）… */
union fuse_file_args {
	/* Used during open() */
	struct fuse_open_out open_outarg;
	/* Used during release() */
	struct fuse_release_args release_args;
};
/* …（同文件无关代码省略）… */
struct fuse_io_priv {
	struct kref refcnt;
	struct work_struct work;
	int async;
	spinlock_t lock;
	unsigned reqs;
	ssize_t bytes;
	size_t size;
	__u64 offset;
	bool write;
	bool should_dirty;
	int err;
	struct kiocb *iocb;
	struct completion *done;
	bool blocking;
};
/* …（同文件无关代码省略）… */
	.iocb = i,			\
}

enum fuse_dax_mode {
	FUSE_DAX_INODE_DEFAULT,	/* default */
	FUSE_DAX_ALWAYS,	/* "-o dax=always" */
	FUSE_DAX_NEVER,		/* "-o dax=never" */
	FUSE_DAX_INODE_USER,	/* "-o dax=inode" */
};

static inline bool fuse_is_inode_dax_mode(enum fuse_dax_mode mode)
{
	return mode == FUSE_DAX_INODE_DEFAULT || mode == FUSE_DAX_INODE_USER;
}

struct fuse_fs_context {
/* …（同文件无关代码省略）… */
	bool no_control:1;
	bool no_force_umount:1;
	bool legacy_opts_show:1;
	enum fuse_dax_mode dax_mode;
	unsigned int max_read;
	unsigned int blksize;
	const char *subtype;

	/* DAX device, may be NULL */
	struct dax_device *dax_dev;
};

struct fuse_sync_bucket {
/* …（同文件无关代码省略）… */
struct fuse_conn {
	/**
	 * @lock: Lock protecting:
	 * - polled_files
	 * - backing_files_map
	 * - curr_bucket
	 */
	spinlock_t lock;

	/** @count: Refcount */
	refcount_t count;

	/** @epoch: Current epoch for up-to-date dentries */
	atomic_t epoch;

	/** @epoch_work: Used to invalidate dentries from old epochs */
	struct work_struct epoch_work;

	/** @rcu: Used to delay freeing fuse_conn, making it safe */
	struct rcu_head rcu;

	/** @user_id: The user id for this mount */
	kuid_t user_id;

	/** @group_id: The group id for this mount */
	kgid_t group_id;

	/** @pid_ns: The pid namespace for this mount */
	struct pid_namespace *pid_ns;

	/** @user_ns: The user namespace for this mount */
	struct user_namespace *user_ns;

	/** @max_read: Maximum read size */
	unsigned max_read;

	/** @max_write: Maximum write size */
	unsigned max_write;

	/**
	 * @max_pages: Maximum number of pages that can be used in a
	 * single request
	 */
	unsigned int max_pages;

	/**
	 * @max_pages_limit: Constrain ->max_pages to this value during
	 * feature negotiation
	 */
	unsigned int max_pages_limit;

	/** @chan: transport layer object */
	struct fuse_chan *chan;

	/** @khctr: The next unique kernel file handle */
	atomic64_t khctr;

	/**
	 * @polled_files: rbtree of fuse_files waiting for poll events
	 * indexed by ph
	 */
	struct rb_root polled_files;

	/**
	 * @congestion_threshold: Number of background requests at which
	 * congestion starts
	 */
	unsigned congestion_threshold;

	/**
	 * @conn_error: Connection failed (version mismatch).  Cannot race with
	 * setting other bitfields since it is only set once in INIT
	 * reply, before any other request, and never cleared
	 */
	unsigned conn_error:1;

	/** @conn_init: Connection successful.  Only set in INIT */
	unsigned conn_init:1;

	/** @async_read: Do readahead asynchronously?  Only set in INIT */
	unsigned async_read:1;

	/**
	 * @abort_err: Return an unique read error after abort.
	 * Only set in INIT
	 */
	unsigned abort_err:1;

	/**
	 * @atomic_o_trunc: Do not send separate SETATTR request before
	 * open(O_TRUNC)
	 */
	unsigned atomic_o_trunc:1;

	/**
	 * @export_support: Filesystem supports NFS exporting.
	 * Only set in INIT
	 */
	unsigned export_support:1;

	/** @writeback_cache: write-back cache policy (default is write-through) */
	unsigned writeback_cache:1;

	/**
	 * @parallel_dirops: allow parallel lookups and readdir (default is
	 * serialized)
	 */
	unsigned parallel_dirops:1;

	/**
	 * @handle_killpriv: handle fs handles killing suid/sgid/cap on
	 * write/chown/trunc
	 */
	unsigned handle_killpriv:1;

	/** @cache_symlinks: cache READLINK responses in page cache */
	unsigned cache_symlinks:1;

	/** @legacy_opts_show: show legacy mount options */
	unsigned int legacy_opts_show:1;

	/**
	 * @handle_killpriv_v2:
	 * fs kills suid/sgid/cap on write/chown/trunc. suid is killed on
	 * write/trunc only if caller did not have CAP_FSETID.  sgid is killed
	 * on write/truncate only if caller did not have CAP_FSETID as well as
	 * file has group execute permission.
	 */
	unsigned handle_killpriv_v2:1;

	/*
	 * The following bitfields are only for optimization purposes
	 * and hence races in setting them will not cause malfunction
	 */

	/** @no_open: Is open/release not implemented by fs? */
	unsigned no_open:1;

	/** @no_opendir: Is opendir/releasedir not implemented by fs? */
	unsigned no_opendir:1;

	/** @no_fsync: Is fsync not implemented by fs? */
	unsigned no_fsync:1;

	/** @no_fsyncdir: Is fsyncdir not implemented by fs? */
	unsigned no_fsyncdir:1;

	/** @no_flush: Is flush not implemented by fs? */
	unsigned no_flush:1;

	/** @no_setxattr: Is setxattr not implemented by fs? */
	unsigned no_setxattr:1;

	/** @setxattr_ext: Does file server support extended setxattr */
	unsigned setxattr_ext:1;

	/** @no_getxattr: Is getxattr not implemented by fs? */
	unsigned no_getxattr:1;

	/** @no_listxattr: Is listxattr not implemented by fs? */
	unsigned no_listxattr:1;

	/** @no_removexattr: Is removexattr not implemented by fs? */
	unsigned no_removexattr:1;

	/** @no_lock: Are posix file locking primitives not implemented by fs? */
	unsigned no_lock:1;

	/** @no_access: Is access not implemented by fs? */
	unsigned no_access:1;

	/** @no_create: Is create not implemented by fs? */
	unsigned no_create:1;

	/** @no_bmap: Is bmap not implemented by fs? */
	unsigned no_bmap:1;

	/** @no_poll: Is poll not implemented by fs? */
	unsigned no_poll:1;

	/** @big_writes: Do multi-page cached writes */
	unsigned big_writes:1;

	/** @dont_mask: Don't apply umask to creation modes */
	unsigned dont_mask:1;

	/** @no_flock: Are BSD file locking primitives not implemented by fs? */
	unsigned no_flock:1;

	/** @no_fallocate: Is fallocate not implemented by fs? */
	unsigned no_fallocate:1;

	/** @no_rename2: Is rename with flags implemented by fs? */
	unsigned no_rename2:1;

	/** @auto_inval_data: Use enhanced/automatic page cache invalidation. */
	unsigned auto_inval_data:1;

	/**
	 * @explicit_inval_data: Filesystem is fully responsible for page cache
	 * invalidation.
	 */
	unsigned explicit_inval_data:1;

	/** @do_readdirplus: Does the filesystem support readdirplus? */
	unsigned do_readdirplus:1;

	/** @readdirplus_auto: Does the filesystem want adaptive readdirplus? */
	unsigned readdirplus_auto:1;

	/**
	 * @async_dio: Does the filesystem support asynchronous direct-IO
	 * submission?
	 */
	unsigned async_dio:1;

	/** @no_lseek: Is lseek not implemented by fs? */
	unsigned no_lseek:1;

	/** @posix_acl: Does the filesystem support posix acls? */
	unsigned posix_acl:1;

	/**
	 * @default_permissions: Check permissions based on the file mode
	 * or not?
	 */
	unsigned default_permissions:1;

	/**
	 * @allow_other: Allow other than the mounter user to access the
	 * filesystem ?
	 */
	unsigned allow_other:1;

	/** @no_copy_file_range: Does the filesystem support copy_file_range? */
	unsigned no_copy_file_range:1;

	/**
	 * @no_copy_file_range_64: Does the filesystem support
	 * copy_file_range_64?
	 */
	unsigned no_copy_file_range_64:1;

	/** @destroy: Send DESTROY request */
	unsigned int destroy:1;

	/** @delete_stale: Delete dentries that have gone stale */
	unsigned int delete_stale:1;

	/** @no_control: Do not create entry in fusectl fs */
	unsigned int no_control:1;

	/** @no_force_umount: Do not allow MNT_FORCE umount */
	unsigned int no_force_umount:1;

	/** @auto_submounts: Auto-mount submounts announced by the server */
	unsigned int auto_submounts:1;

	/** @sync_fs: Propagate syncfs() to server */
	unsigned int sync_fs:1;

	/** @init_security: Initialize security xattrs when creating a new inode */
	unsigned int init_security:1;

	/**
	 * @create_supp_group: Add supplementary group info when creating
	 * a new inode
	 */
	unsigned int create_supp_group:1;

	/** @inode_dax: Does the filesystem support per inode DAX? */
	unsigned int inode_dax:1;

	/** @no_tmpfile: Is tmpfile not implemented by fs? */
	unsigned int no_tmpfile:1;

	/**
	 * @direct_io_allow_mmap: Relax restrictions to allow shared mmap
	 * in FOPEN_DIRECT_IO mode
	 */
	unsigned int direct_io_allow_mmap:1;

	/** @no_statx: Is statx not implemented by fs? */
	unsigned int no_statx:1;

	/** @passthrough: Passthrough support for read/write IO */
	unsigned int passthrough:1;

	/** @use_pages_for_kvec_io: Use pages instead of pointer for kernel I/O */
	unsigned int use_pages_for_kvec_io:1;

	/** @no_link: Is link not implemented by fs? */
	unsigned int no_link:1;

	/** @sync_init: Is synchronous FUSE_INIT allowed? */
	unsigned int sync_init:1;

	/** @max_stack_depth: Maximum stack depth for passthrough backing files */
	int max_stack_depth;

	/** @minor: Negotiated minor version */
	unsigned minor;

	/** @entry: Entry on the fuse_conn_list */
	struct list_head entry;

	/** @dev: Device ID from the root super block */
	dev_t dev;

	/** @scramble_key: Key for lock owner ID scrambling */
	u32 scramble_key[4];

	/** @attr_version: Version counter for attribute changes */
	atomic64_t attr_version;

	/** @evict_ctr: Version counter for evict inode */
	atomic64_t evict_ctr;

	/** @name_max: maximum file name length */
	u32 name_max;

	/** @release: Called on final put */
	void (*release)(struct fuse_conn *);

	/**
	 * @killsb: Read/write semaphore to hold when accessing the sb of any
	 * fuse_mount belonging to this connection
	 */
	struct rw_semaphore killsb;

#ifdef CONFIG_FUSE_DAX
	/** @dax_mode: Dax mode */
	enum fuse_dax_mode dax_mode;

	/** @dax: Dax specific conn data, non-NULL if DAX is enabled */
	struct fuse_conn_dax *dax;
#endif

	/** @mounts: List of filesystems using this connection */
	struct list_head mounts;

	/** @curr_bucket: New writepages go into this bucket */
	struct fuse_sync_bucket __rcu *curr_bucket;

#ifdef CONFIG_FUSE_PASSTHROUGH
	/** @backing_files_map: IDR for backing files ids */
	struct idr backing_files_map;
#endif
};

/* …（同文件无关代码省略）… */
struct fuse_mount {
	/* Underlying (potentially shared) connection to the FUSE server */
	struct fuse_conn *fc;

	/*
	 * Super block for this connection (fc->killsb must be held when
	 * accessing this).
	 */
	struct super_block *sb;

	/* Entry on fc->mounts */
	struct list_head fc_entry;
	struct rcu_head rcu;
};
/* …（同文件无关代码省略）… */
		 bool isdir);

/*
 * fuse_direct_io() flags
 */

/** If set, it is WRITE; otherwise - READ */
#define FUSE_DIO_WRITE (1 << 0)

/** CUSE pass fuse_direct_io() a file which f_mapping->host is not from FUSE */
#define FUSE_DIO_CUSE  (1 << 1)

ssize_t fuse_direct_io(struct fuse_io_priv *io, struct iov_iter *iter,
		       loff_t *ppos, int flags);
long fuse_do_ioctl(struct file *file, unsigned int cmd, unsigned long arg,
		   unsigned int flags);
long fuse_ioctl_common(struct file *file, unsigned int cmd,
/* …（同文件无关代码省略）… */

/* dax.c */

#define FUSE_IS_DAX(inode) (IS_ENABLED(CONFIG_FUSE_DAX) && IS_DAX(inode))

ssize_t fuse_dax_read_iter(struct kiocb *iocb, struct iov_iter *to);
ssize_t fuse_dax_write_iter(struct kiocb *iocb, struct iov_iter *from);
int fuse_dax_mmap(struct file *file, struct vm_area_struct *vma);
int fuse_dax_break_layouts(struct inode *inode, u64 dmap_start, u64 dmap_end);
int fuse_dax_conn_alloc(struct fuse_conn *fc, enum fuse_dax_mode mode,
			struct dax_device *dax_dev);
void fuse_dax_conn_free(struct fuse_conn *fc);
bool fuse_dax_inode_alloc(struct super_block *sb, struct fuse_inode *fi);
void fuse_dax_inode_init(struct inode *inode, unsigned int flags);
void fuse_dax_inode_cleanup(struct inode *inode);
void fuse_dax_dontcache(struct inode *inode, unsigned int flags);
bool fuse_dax_check_alignment(struct fuse_conn *fc, unsigned int map_alignment);
void fuse_dax_cancel_work(struct fuse_conn *fc);

/* ioctl.c */
long fuse_file_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
/* …（同文件无关代码省略）… */
		      struct dentry *dentry, struct file_kattr *fa);

/* iomode.c */
int fuse_file_cached_io_open(struct inode *inode, struct fuse_file *ff);
int fuse_inode_uncached_io_start(struct fuse_inode *fi,
				 struct fuse_backing *fb);
void fuse_inode_uncached_io_end(struct fuse_inode *fi);
/* …（同文件无关代码省略）… */

/* backing.c */
#ifdef CONFIG_FUSE_PASSTHROUGH
struct fuse_backing *fuse_backing_get(struct fuse_backing *fb);
void fuse_backing_put(struct fuse_backing *fb);
struct fuse_backing *fuse_backing_lookup(struct fuse_conn *fc, int backing_id);
#else

static inline struct fuse_backing *fuse_backing_get(struct fuse_backing *fb)
{
	return NULL;
}

static inline void fuse_backing_put(struct fuse_backing *fb)
{
}
static inline struct fuse_backing *fuse_backing_lookup(struct fuse_conn *fc,
						       int backing_id)
{
	return NULL;
}
#endif

void fuse_backing_files_init(struct fuse_conn *fc);
void fuse_backing_files_free(struct fuse_conn *fc);

/* passthrough.c */
/* …（同文件无关代码省略）… */
#endif
}

struct fuse_backing *fuse_passthrough_open(struct file *file, int backing_id);
void fuse_passthrough_release(struct fuse_file *ff, struct fuse_backing *fb);

static inline struct file *fuse_file_passthrough(struct fuse_file *ff)
{
#ifdef CONFIG_FUSE_PASSTHROUGH
	return ff->passthrough;
#else
	return NULL;
#endif
}

ssize_t fuse_passthrough_read_iter(struct kiocb *iocb, struct iov_iter *iter);
/* …（同文件无关代码省略）… */
				      struct file *out, loff_t *ppos,
				      size_t len, unsigned int flags);
ssize_t fuse_passthrough_mmap(struct file *file, struct vm_area_struct *vma);

#ifdef CONFIG_SYSCTL
extern int fuse_sysctl_register(void);
/* …（同文件无关代码省略）… */
#define fuse_sysctl_register()		(0)
#define fuse_sysctl_unregister()	do { } while (0)
#endif /* CONFIG_SYSCTL */

#endif /* _FS_FUSE_I_H */
