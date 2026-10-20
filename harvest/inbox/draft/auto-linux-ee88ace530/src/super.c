// AUTO-DRAFT from torvalds/linux PR #58aeff92eebcad28da1f01e6d16b04794f7ea4f2
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static inline void super_unlock(struct super_block *sb, bool excl)
{
	if (excl)
		up_write(&sb->s_umount);
	else
		up_read(&sb->s_umount);
}
/* …（同文件无关代码省略）… */
static inline void super_unlock_excl(struct super_block *sb)
{
	super_unlock(sb, true);
}
/* …（同文件无关代码省略）… */
static inline void super_unlock_shared(struct super_block *sb)
{
	super_unlock(sb, false);
}
/* …（同文件无关代码省略）… */
static bool super_flags(const struct super_block *sb, unsigned int flags)
{
	/*
	 * Pairs with smp_store_release() in super_wake() and ensures
	 * that we see @flags after we're woken.
	 */
	return smp_load_acquire(&sb->s_flags) & flags;
}
/* …（同文件无关代码省略）… */
static __must_check bool super_lock(struct super_block *sb, bool excl)
{
	lockdep_assert_not_held(&sb->s_umount);

	/* wait until the superblock is ready or dying */
	wait_var_event(&sb->s_flags, super_flags(sb, SB_BORN | SB_DYING));

	/* Don't pointlessly acquire s_umount. */
	if (super_flags(sb, SB_DYING))
		return false;

	__super_lock(sb, excl);

	/*
	 * Has gone through generic_shutdown_super() in the meantime.
	 * @sb->s_root is NULL and @sb->s_active is 0. No one needs to
	 * grab a reference to this. Tell them so.
	 */
	if (sb->s_flags & SB_DYING) {
		super_unlock(sb, excl);
		return false;
	}

	WARN_ON_ONCE(!(sb->s_flags & SB_BORN));
	return true;
}
/* …（同文件无关代码省略）… */
static inline bool super_lock_excl(struct super_block *sb)
{
	return super_lock(sb, true);
}
/* …（同文件无关代码省略）… */
#define SUPER_WAKE_FLAGS (SB_BORN | SB_DYING | SB_DEAD)
static void super_wake(struct super_block *sb, unsigned int flag)
{
	WARN_ON_ONCE((flag & ~SUPER_WAKE_FLAGS));
	WARN_ON_ONCE(hweight32(flag & SUPER_WAKE_FLAGS) > 1);

	/*
	 * Pairs with smp_load_acquire() in super_lock() to make sure
	 * all initializations in the superblock are seen by the user
	 * seeing SB_BORN sent.
	 */
	smp_store_release(&sb->s_flags, sb->s_flags | flag);
	/*
	 * Pairs with the barrier in prepare_to_wait_event() to make sure
	 * ___wait_var_event() either sees SB_BORN set or
	 * waitqueue_active() check in wake_up_var() sees the waiter.
	 */
	smp_mb();
	wake_up_var(&sb->s_flags);
}
/* …（同文件无关代码省略）… */
static unsigned long super_cache_scan(struct shrinker *shrink,
				      struct shrink_control *sc)
{
	struct super_block *sb;
	long	fs_objects = 0;
	long	total_objects;
	long	freed = 0;
	long	dentries;
	long	inodes;

	sb = shrink->private_data;

	/*
	 * Deadlock avoidance.  We may hold various FS locks, and we don't want
	 * to recurse into the FS that called us in clear_inode() and friends..
	 */
	if (!(sc->gfp_mask & __GFP_FS))
		return SHRINK_STOP;

	if (!super_trylock_shared(sb))
		return SHRINK_STOP;

	if (sb->s_op->nr_cached_objects)
		fs_objects = sb->s_op->nr_cached_objects(sb, sc);

	inodes = list_lru_shrink_count(&sb->s_inode_lru, sc);
	dentries = list_lru_shrink_count(&sb->s_dentry_lru, sc);
	total_objects = dentries + inodes + fs_objects;
	if (!total_objects)
		total_objects = 1;

	/* proportion the scan between the caches */
	dentries = mult_frac(sc->nr_to_scan, dentries, total_objects);
	inodes = mult_frac(sc->nr_to_scan, inodes, total_objects);
	fs_objects = mult_frac(sc->nr_to_scan, fs_objects, total_objects);

	/*
	 * prune the dcache first as the icache is pinned by it, then
	 * prune the icache, followed by the filesystem specific caches
	 *
	 * Ensure that we always scan at least one object - memcg kmem
	 * accounting uses this to fully empty the caches.
	 */
	sc->nr_to_scan = dentries + 1;
	freed = prune_dcache_sb(sb, sc);
	sc->nr_to_scan = inodes + 1;
	freed += prune_icache_sb(sb, sc);

	if (fs_objects) {
		sc->nr_to_scan = fs_objects + 1;
		freed += sb->s_op->free_cached_objects(sb, sc);
	}

	super_unlock_shared(sb);
	return freed;
}
/* …（同文件无关代码省略）… */
static unsigned long super_cache_count(struct shrinker *shrink,
				       struct shrink_control *sc)
{
	struct super_block *sb;
	long	total_objects = 0;

	sb = shrink->private_data;

	/*
	 * We don't call super_trylock_shared() here as it is a scalability
	 * bottleneck, so we're exposed to partial setup state. The shrinker
	 * rwsem does not protect filesystem operations backing
	 * list_lru_shrink_count() or s_op->nr_cached_objects(). Counts can
	 * change between super_cache_count and super_cache_scan, so we really
	 * don't need locks here.
	 *
	 * However, if we are currently mounting the superblock, the underlying
	 * filesystem might be in a state of partial construction and hence it
	 * is dangerous to access it.  super_trylock_shared() uses a SB_BORN check
	 * to avoid this situation, so do the same here. The memory barrier is
	 * matched with the one in mount_fs() as we don't hold locks here.
	 */
	if (!(sb->s_flags & SB_BORN))
		return 0;
	smp_rmb();

	if (sb->s_op && sb->s_op->nr_cached_objects)
		total_objects = sb->s_op->nr_cached_objects(sb, sc);

	total_objects += list_lru_shrink_count(&sb->s_dentry_lru, sc);
	total_objects += list_lru_shrink_count(&sb->s_inode_lru, sc);

	if (!total_objects)
		return SHRINK_EMPTY;

	total_objects = vfs_pressure_ratio(total_objects);
	return total_objects;
}
/* …（同文件无关代码省略）… */
static void destroy_super_work(struct work_struct *work)
{
	struct super_block *s = container_of(work, struct super_block,
							destroy_work);
	fsnotify_sb_free(s);
	security_sb_free(s);
	put_user_ns(s->s_user_ns);
	/* Only an unregistered entry is still owned by the superblock. */
	kfree(s->s_super_dev);
	kfree(s->s_subtype);
	for (int i = 0; i < SB_FREEZE_LEVELS; i++)
		percpu_free_rwsem(&s->s_writers.rw_sem[i]);
	kfree(s);
}
/* …（同文件无关代码省略）… */
static void destroy_super_rcu(struct rcu_head *head)
{
	struct super_block *s = container_of(head, struct super_block, rcu);
	INIT_WORK(&s->destroy_work, destroy_super_work);
	schedule_work(&s->destroy_work);
}
/* …（同文件无关代码省略）… */
static void destroy_unused_super(struct super_block *s)
{
	if (!s)
		return;
	super_unlock_excl(s);
	list_lru_destroy(&s->s_dentry_lru);
	list_lru_destroy(&s->s_inode_lru);
	shrinker_free(s->s_shrink);
	/* no delays needed */
	destroy_super_work(&s->destroy_work);
}
/* …（同文件无关代码省略）… */
static struct super_block *alloc_super(struct file_system_type *type, int flags,
				       struct user_namespace *user_ns)
{
	struct super_block *s = kzalloc_obj(struct super_block);
	static const struct super_operations default_op;
	int i;

	if (!s)
		return NULL;

	s->s_user_ns = get_user_ns(user_ns);
	init_rwsem(&s->s_umount);
	lockdep_set_class(&s->s_umount, &type->s_umount_key);
	/*
	 * sget_fc() can have s_umount recursion.
	 *
	 * When it cannot find a suitable sb, it allocates a new
	 * one (this one), and tries again to find a suitable old
	 * one.
	 *
	 * In case that succeeds, it will acquire the s_umount
	 * lock of the old one. Since these are clearly distrinct
	 * locks, and this object isn't exposed yet, there's no
	 * risk of deadlocks.
	 *
	 * Annotate this by putting this lock in a different
	 * subclass.
	 */
	down_write_nested(&s->s_umount, SINGLE_DEPTH_NESTING);

	if (security_sb_alloc(s))
		goto fail;

	for (i = 0; i < SB_FREEZE_LEVELS; i++) {
		if (__percpu_init_rwsem(&s->s_writers.rw_sem[i],
					sb_writers_name[i],
					&type->s_writers_key[i]))
			goto fail;
	}
	s->s_bdi = &noop_backing_dev_info;
	s->s_flags = flags;
	if (s->s_user_ns != &init_user_ns)
		s->s_iflags |= SB_I_NODEV;
	INIT_HLIST_NODE(&s->s_instances);
	INIT_HLIST_BL_HEAD(&s->s_roots);
	spin_lock_init(&s->s_roots_lock);
	mutex_init(&s->s_sync_lock);
	INIT_LIST_HEAD(&s->s_inodes);
	spin_lock_init(&s->s_inode_list_lock);
	INIT_LIST_HEAD(&s->s_inodes_wb);
	spin_lock_init(&s->s_inode_wblist_lock);
	fserror_mount(s);

	refcount_set(&s->s_passive, 1);
	atomic_set(&s->s_active, 1);
	mutex_init(&s->s_vfs_rename_mutex);
	lockdep_set_class(&s->s_vfs_rename_mutex, &type->s_vfs_rename_key);
	init_rwsem(&s->s_dquot.dqio_sem);
	s->s_maxbytes = MAX_NON_LFS;
	s->s_op = &default_op;
	s->s_time_gran = 1000000000;
	s->s_time_min = TIME64_MIN;
	s->s_time_max = TIME64_MAX;

	s->s_shrink = shrinker_alloc(SHRINKER_NUMA_AWARE | SHRINKER_MEMCG_AWARE,
				     "sb-%s", type->name);
	if (!s->s_shrink)
		goto fail;

	s->s_shrink->scan_objects = super_cache_scan;
	s->s_shrink->count_objects = super_cache_count;
	s->s_shrink->batch = 1024;
	s->s_shrink->private_data = s;

	if (list_lru_init_memcg(&s->s_dentry_lru, s->s_shrink))
		goto fail;
	if (list_lru_init_memcg(&s->s_inode_lru, s->s_shrink))
		goto fail;
	s->s_super_dev = super_dev_alloc(0, s);
	if (!s->s_super_dev)
		goto fail;

	s->s_min_writeback_pages = MIN_WRITEBACK_PAGES;
	return s;

fail:
	destroy_unused_super(s);
	return NULL;
}
/* …（同文件无关代码省略）… */
void put_super(struct super_block *s)
{
	if (refcount_dec_and_test(&s->s_passive)) {
		struct file_system_type *type = s->s_type;

		spin_lock(&sb_lock);
		list_del_init(&s->s_list);
		hlist_del_init(&s->s_instances);
		spin_unlock(&sb_lock);

		WARN_ON(s->s_dentry_lru.node);
		WARN_ON(s->s_inode_lru.node);
		WARN_ON(s->s_mounts);
		call_rcu(&s->rcu, destroy_super_rcu);
		/* The unlink above may touch type->fs_supers, so drop it last. */
		put_filesystem(type);
	}
}
/* …（同文件无关代码省略）… */
struct super_dev {
	dev_t			sd_dev;
	struct super_block	*sd_sb;
	refcount_t		sd_ref;
	struct rhlist_head	sd_node;
	struct rcu_head		sd_rcu;
};
/* …（同文件无关代码省略）… */
static struct super_dev *super_dev_alloc(dev_t dev, struct super_block *sb)
{
	struct super_dev *fsd;

	fsd = kzalloc_obj(*fsd);
	if (!fsd)
		return NULL;
	fsd->sd_dev = dev;
	fsd->sd_sb = sb;
	refcount_set(&fsd->sd_ref, 1);
	return fsd;
}
/* …（同文件无关代码省略）… */
static void super_dev_put(struct super_dev *fsd)
{
	/* Unlink only once unpinned, so a cursor never resumes from a removed node. */
	if (fsd && refcount_dec_and_test(&fsd->sd_ref)) {
		rhltable_remove(&super_dev_table, &fsd->sd_node, super_dev_params);
		put_super(fsd->sd_sb);
		kfree_rcu(fsd, sd_rcu);
	}
}
/* …（同文件无关代码省略）… */
static int super_dev_insert(struct super_dev *fsd)
{
	int err;

	err = rhltable_insert(&super_dev_table, &fsd->sd_node, super_dev_params);
	if (!err)
		refcount_inc(&fsd->sd_sb->s_passive);
	return err;
}
/* …（同文件无关代码省略）… */
static int super_dev_register(struct super_block *sb)
{
	struct super_dev *fsd = sb->s_super_dev;
	int err;

	lockdep_assert_held(&sb_lock);
	VFS_WARN_ON_ONCE(!sb->s_dev);
	VFS_WARN_ON_ONCE(!fsd || fsd->sd_dev);

	fsd->sd_dev = sb->s_dev;
	err = super_dev_insert(fsd);
	if (err)
		fsd->sd_dev = 0;
	return err;
}
/* …（同文件无关代码省略）… */
static void kill_super_notify(struct super_block *sb)
{
	lockdep_assert_not_held(&sb->s_umount);

	/* already notified earlier */
	if (sb->s_flags & SB_DEAD)
		return;

	/* Drop sget_fc()'s claim; a never-registered entry stays with the sb. */
	if (sb->s_super_dev->sd_dev) {
		super_dev_put(sb->s_super_dev);
		sb->s_super_dev = NULL;
	}

	/*
	 * Let concurrent mounts know that this thing is really dead.
	 * sget_fc() skips SB_DEAD superblocks and calls test() under
	 * sb_lock, so set it under sb_lock: once we return no test()
	 * runs on this superblock anymore and none will start. Everyone
	 * else already saw SB_DYING and either discarded the superblock
	 * or waits for SB_DEAD.
	 */
	spin_lock(&sb_lock);
	super_wake(sb, SB_DEAD);
	spin_unlock(&sb_lock);
}
/* …（同文件无关代码省略）… */
void deactivate_locked_super(struct super_block *s)
{
	struct file_system_type *fs = s->s_type;
	if (atomic_dec_and_test(&s->s_active)) {
		shrinker_free(s->s_shrink);
		fs->kill_sb(s);

		kill_super_notify(s);

		/* list_lru_destroy() may sleep; put_super() callers may not. */
		list_lru_destroy(&s->s_dentry_lru);
		list_lru_destroy(&s->s_inode_lru);

		put_super(s);
	} else {
		super_unlock_excl(s);
	}
}
/* …（同文件无关代码省略）… */
static bool grab_super(struct super_block *sb)
{
	bool locked;

	refcount_inc(&sb->s_passive);
	spin_unlock(&sb_lock);
	locked = super_lock_excl(sb);
	if (locked) {
		if (atomic_inc_not_zero(&sb->s_active)) {
			put_super(sb);
			return true;
		}
		super_unlock_excl(sb);
	}
	wait_var_event(&sb->s_flags, super_flags(sb, SB_DEAD));
	put_super(sb);
	return false;
}
/* …（同文件无关代码省略）… */
bool super_trylock_shared(struct super_block *sb)
{
	if (down_read_trylock(&sb->s_umount)) {
		if (!(sb->s_flags & SB_DYING) && sb->s_root &&
		    (sb->s_flags & SB_BORN))
			return true;
		super_unlock_shared(sb);
	}

	return false;
}
/* …（同文件无关代码省略）… */
struct super_block *sget_fc(struct fs_context *fc,
			    int (*test)(struct super_block *, struct fs_context *),
			    int (*set)(struct super_block *, struct fs_context *))
{
	struct super_block *s = NULL;
	struct super_block *old;
	struct user_namespace *user_ns = fc->global ? &init_user_ns : fc->user_ns;
	int err;

	/*
	 * Never allow s_user_ns != &init_user_ns when FS_USERNS_MOUNT or
	 * FS_USERNS_DELEGATABLE is not set, as the filesystem is likely
	 * unprepared to handle it. This can happen when fsconfig() is called
	 * from init_user_ns with an fs_fd opened in another user namespace.
	 */
	if (user_ns != &init_user_ns &&
	    !(fc->fs_type->fs_flags & (FS_USERNS_MOUNT | FS_USERNS_DELEGATABLE))) {
		errorfc(fc, "VFS: Mounting from non-initial user namespace is not allowed");
		return ERR_PTR(-EPERM);
	}

retry:
	spin_lock(&sb_lock);
	if (test) {
		hlist_for_each_entry(old, &fc->fs_type->fs_supers, s_instances) {
			/* Only unlinked at the last passive reference. */
			if (super_flags(old, SB_DEAD))
				continue;
			if (test(old, fc))
				goto share_extant_sb;
		}
	}
	if (!s) {
		spin_unlock(&sb_lock);

		s = alloc_super(fc->fs_type, fc->sb_flags, user_ns);
		if (!s)
			return ERR_PTR(-ENOMEM);
		goto retry;
	}

	s->s_fs_info = fc->s_fs_info;
	err = set(s, fc);
	if (err) {
		VFS_WARN_ON_ONCE(s->s_super_dev->sd_dev);
		s->s_fs_info = NULL;
		spin_unlock(&sb_lock);
		destroy_unused_super(s);
		return ERR_PTR(err);
	}
	VFS_WARN_ON_ONCE(!s->s_super_dev->sd_dev);
	fc->s_fs_info = NULL;
	s->s_type = fc->fs_type;
	s->s_iflags |= fc->s_iflags;
	strscpy(s->s_id, s->s_type->name, sizeof(s->s_id));
	/*
	 * Make the superblock visible on @super_blocks and @fs_supers.
	 * It's in a nascent state and users should wait on SB_BORN or
	 * SB_DYING to be set.
	 */
	list_add_tail(&s->s_list, &super_blocks);
	hlist_add_head(&s->s_instances, &s->s_type->fs_supers);
	spin_unlock(&sb_lock);
	get_filesystem(s->s_type);
	shrinker_register(s->s_shrink);
	return s;

share_extant_sb:
	if (user_ns != old->s_user_ns || fc->exclusive) {
		spin_unlock(&sb_lock);
		destroy_unused_super(s);
		if (fc->exclusive)
			warnfc(fc, "reusing existing filesystem not allowed");
		else
			warnfc(fc, "reusing existing filesystem in another namespace not allowed");
		return ERR_PTR(-EBUSY);
	}
	if (!grab_super(old))
		goto retry;
	destroy_unused_super(s);
	return old;
}
/* …（同文件无关代码省略）… */
int get_anon_bdev(dev_t *p)
{
	int dev;

	/*
	 * Many userspace utilities consider an FSID of 0 invalid.
	 * Always return at least 1 from get_anon_bdev.
	 */
	dev = ida_alloc_range(&unnamed_dev_ida, 1, (1 << MINORBITS) - 1,
			GFP_ATOMIC);
	if (dev == -ENOSPC)
		dev = -EMFILE;
	if (dev < 0)
		return dev;

	*p = MKDEV(0, dev);
	return 0;
}
/* …（同文件无关代码省略）… */
void free_anon_bdev(dev_t dev)
{
	ida_free(&unnamed_dev_ida, MINOR(dev));
}
/* …（同文件无关代码省略）… */
int set_anon_super(struct super_block *s, void *data)
{
	int error;

	error = get_anon_bdev(&s->s_dev);
	if (error)
		return error;

	error = super_dev_register(s);
	if (error)
		free_anon_bdev(s->s_dev);
	return error;
}
/* …（同文件无关代码省略）… */
int set_anon_super_fc(struct super_block *sb, struct fs_context *fc)
{
	return set_anon_super(sb, NULL);
}
/* …（同文件无关代码省略）… */
static int test_keyed_super(struct super_block *sb, struct fs_context *fc)
{
	return sb->s_fs_info == fc->s_fs_info;
}
/* …（同文件无关代码省略）… */
static int test_single_super(struct super_block *s, struct fs_context *fc)
{
	return 1;
}

static int vfs_get_super(struct fs_context *fc,
		int (*test)(struct super_block *, struct fs_context *),
		int (*fill_super)(struct super_block *sb,
				  struct fs_context *fc))
{
	struct super_block *sb;
	int err;

	sb = sget_fc(fc, test, set_anon_super_fc);
	if (IS_ERR(sb))
		return PTR_ERR(sb);

	if (!sb->s_root) {
		err = fill_super(sb, fc);
		if (err)
			goto error;

		sb->s_flags |= SB_ACTIVE;
	}

	fc->root = dget(sb->s_root);
	return 0;

error:
	deactivate_locked_super(sb);
	return err;
}

int get_tree_nodev(struct fs_context *fc,
		  int (*fill_super)(struct super_block *sb,
				    struct fs_context *fc))
{
	return vfs_get_super(fc, NULL, fill_super);
}
EXPORT_SYMBOL(get_tree_nodev);

int get_tree_single(struct fs_context *fc,
		  int (*fill_super)(struct super_block *sb,
				    struct fs_context *fc))
{
	return vfs_get_super(fc, test_single_super, fill_super);
}
EXPORT_SYMBOL(get_tree_single);

int get_tree_keyed(struct fs_context *fc,
		  int (*fill_super)(struct super_block *sb,
				    struct fs_context *fc),
		void *key)
{
	fc->s_fs_info = key;
	return vfs_get_super(fc, test_keyed_super, fill_super);
}
EXPORT_SYMBOL(get_tree_keyed);
