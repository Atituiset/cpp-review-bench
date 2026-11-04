// AUTO-DRAFT from torvalds/linux PR #51628610f9187c1c1cfe499d92e066bc22ad9693
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
	return dentry->d_name.name != dentry->d_shortname.string;
}

static bool __take_dentry_name_snapshot(struct name_snapshot *name,
					struct dentry *dentry, bool retry)
{
	unsigned seq;
	const unsigned char *s;

	rcu_read_lock();
retry:
	seq = read_seqcount_begin(&dentry->d_seq);
	s = READ_ONCE(dentry->d_name.name);
	name->name.hash_len = dentry->d_name.hash_len;
	name->name.name = name->inline_name.string;
	if (likely(s == dentry->d_shortname.string)) {
		name->inline_name = dentry->d_shortname;
	} else {
		struct external_name *p;
		p = container_of(s, struct external_name, name[0]);
		// get a valid reference
		if (unlikely(!atomic_inc_not_zero(&p->count))) {
			if (retry)
				goto retry;
			rcu_read_unlock();
			return false;
		}
		name->name.name = s;
	}
	if (read_seqcount_retry(&dentry->d_seq, seq)) {
		release_dentry_name_snapshot(name);
		goto retry;
	}
	rcu_read_unlock();
	return true;
}

void take_dentry_name_snapshot(struct name_snapshot *name, struct dentry *dentry)
{
	__take_dentry_name_snapshot(name, dentry, true);
}
EXPORT_SYMBOL(take_dentry_name_snapshot);

/*
 * For a caller that holds no reference on @dentry, e.g. in RCU walk.
 * The external name of a dead dentry has a zero refcount forever, so
 * take_dentry_name_snapshot() would retry without end.  Fail instead.
 */
bool try_take_dentry_name_snapshot(struct name_snapshot *name,
				   struct dentry *dentry)
{
	return __take_dentry_name_snapshot(name, dentry, false);
}
EXPORT_SYMBOL(try_take_dentry_name_snapshot);

void release_dentry_name_snapshot(struct name_snapshot *name)
{
	if (unlikely(name->name.name != name->inline_name.string)) {
