// AUTO-DRAFT from torvalds/linux PR #ca62fc8c6b1841039aee6de1ef03c9863751ec04
	return dentry->d_name.name != dentry->d_shortname.string;
}
  // <<< BUG ANCHOR
void take_dentry_name_snapshot(struct name_snapshot *name, struct dentry *dentry)
{
	unsigned seq;
	const unsigned char *s;
/* …（同文件无关代码省略）… */
		struct external_name *p;
		p = container_of(s, struct external_name, name[0]);
		// get a valid reference
		if (unlikely(!atomic_inc_not_zero(&p->count)))
			goto retry;
		name->name.name = s;
	}
	if (read_seqcount_retry(&dentry->d_seq, seq)) {
		release_dentry_name_snapshot(name);
		goto retry;
	}
	rcu_read_unlock();
}
EXPORT_SYMBOL(take_dentry_name_snapshot);

void release_dentry_name_snapshot(struct name_snapshot *name)
{
	if (unlikely(name->name.name != name->inline_name.string)) {
