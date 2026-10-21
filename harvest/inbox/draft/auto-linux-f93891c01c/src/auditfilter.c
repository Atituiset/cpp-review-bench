// AUTO-DRAFT from torvalds/linux PR #0d508d4709837bcc88e314c0533a42d3c55b31e3
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
static void audit_free_lsm_field(struct audit_field *f)
{
	switch (f->type) {
	case AUDIT_SUBJ_USER:
	case AUDIT_SUBJ_ROLE:
	case AUDIT_SUBJ_TYPE:
	case AUDIT_SUBJ_SEN:
	case AUDIT_SUBJ_CLR:
	case AUDIT_OBJ_USER:
	case AUDIT_OBJ_ROLE:
	case AUDIT_OBJ_TYPE:
	case AUDIT_OBJ_LEV_LOW:
	case AUDIT_OBJ_LEV_HIGH:
		kfree(f->lsm_str);
		security_audit_rule_free(f->lsm_rule);
	}
}
/* …（同文件无关代码省略）… */
static inline void audit_free_rule(struct audit_entry *e)
{
	int i;
	struct audit_krule *erule = &e->rule;

	/* some rules don't have associated watches */
	if (erule->watch)
		audit_put_watch(erule->watch);
	if (erule->fields)
		for (i = 0; i < erule->field_count; i++)
			audit_free_lsm_field(&erule->fields[i]);
	kfree(erule->fields);
	kfree(erule->filterkey);
	kfree(e);
}
/* …（同文件无关代码省略）… */
void audit_free_rule_rcu(struct rcu_head *head)
{
	struct audit_entry *e = container_of(head, struct audit_entry, rcu);
	audit_free_rule(e);
}
/* …（同文件无关代码省略）… */
}

#ifdef CONFIG_AUDITSYSCALL
static inline int audit_match_class_bits(int class, u32 *mask)
{
	int i;

/* …（同文件无关代码省略）… */
	return 1;
}

static int audit_match_signal(struct audit_entry *entry)
{
	struct audit_field *arch = entry->rule.arch_f;

	if (!arch) {
		/* When arch is unspecified, we must check both masks on biarch
		 * as syscall number alone is ambiguous. */
		return (audit_match_class_bits(AUDIT_CLASS_SIGNAL,
					       entry->rule.mask) &&
			audit_match_class_bits(AUDIT_CLASS_SIGNAL_32,
					       entry->rule.mask));
	}

	switch (audit_classify_arch(arch->val)) {
	case 0: /* native */
		return (audit_match_class_bits(AUDIT_CLASS_SIGNAL,
					       entry->rule.mask));
	case 1: /* 32bit on biarch */
		return (audit_match_class_bits(AUDIT_CLASS_SIGNAL_32,
					       entry->rule.mask));
	default:
		return 1;
	}
}
#endif

/* Common user-space to kernel rule translation. */
/* …（同文件无关代码省略）… */
static int audit_compare_rule(struct audit_krule *a, struct audit_krule *b)
{
	int i;

	if (a->flags != b->flags ||
	    a->pflags != b->pflags ||
	    a->listnr != b->listnr ||
	    a->action != b->action ||
	    a->field_count != b->field_count)
		return 1;

	for (i = 0; i < a->field_count; i++) {
		if (a->fields[i].type != b->fields[i].type ||
		    a->fields[i].op != b->fields[i].op)
			return 1;

		switch (a->fields[i].type) {
		case AUDIT_SUBJ_USER:
		case AUDIT_SUBJ_ROLE:
		case AUDIT_SUBJ_TYPE:
		case AUDIT_SUBJ_SEN:
		case AUDIT_SUBJ_CLR:
		case AUDIT_OBJ_USER:
		case AUDIT_OBJ_ROLE:
		case AUDIT_OBJ_TYPE:
		case AUDIT_OBJ_LEV_LOW:
		case AUDIT_OBJ_LEV_HIGH:
			if (strcmp(a->fields[i].lsm_str, b->fields[i].lsm_str))
				return 1;
			break;
		case AUDIT_WATCH:
			if (strcmp(audit_watch_path(a->watch),
				   audit_watch_path(b->watch)))
				return 1;
			break;
		case AUDIT_DIR:
			if (strcmp(audit_tree_path(a->tree),
				   audit_tree_path(b->tree)))
				return 1;
			break;
		case AUDIT_FILTERKEY:
			/* both filterkeys exist based on above type compare */
			if (strcmp(a->filterkey, b->filterkey))
				return 1;
			break;
		case AUDIT_EXE:
			/* both paths exist based on above type compare */
			if (strcmp(audit_mark_path(a->exe),
				   audit_mark_path(b->exe)))
				return 1;
			break;
		case AUDIT_UID:
		case AUDIT_EUID:
		case AUDIT_SUID:
		case AUDIT_FSUID:
		case AUDIT_LOGINUID:
		case AUDIT_OBJ_UID:
			if (!uid_eq(a->fields[i].uid, b->fields[i].uid))
				return 1;
			break;
		case AUDIT_GID:
		case AUDIT_EGID:
		case AUDIT_SGID:
		case AUDIT_FSGID:
		case AUDIT_OBJ_GID:
			if (!gid_eq(a->fields[i].gid, b->fields[i].gid))
				return 1;
			break;
		default:
			if (a->fields[i].val != b->fields[i].val)
				return 1;
		}
	}

	for (i = 0; i < AUDIT_BITMASK_SIZE; i++)
		if (a->mask[i] != b->mask[i])
			return 1;

	return 0;
}
/* …（同文件无关代码省略）… */
static struct audit_entry *audit_find_rule(struct audit_entry *entry,
					   struct list_head **p)
{
	struct audit_entry *e, *found = NULL;
	struct list_head *list;
	int h;

	if (entry->rule.inode_f) {
		h = audit_hash_ino(entry->rule.inode_f->val);
		*p = list = &audit_inode_hash[h];
	} else if (entry->rule.watch) {
		/* we don't know the inode number, so must walk entire hash */
		for (h = 0; h < AUDIT_INODE_BUCKETS; h++) {
			list = &audit_inode_hash[h];
			list_for_each_entry(e, list, list)
				if (!audit_compare_rule(&entry->rule, &e->rule)) {
					found = e;
					goto out;
				}
		}
		goto out;
	} else {
		*p = list = &audit_filter_list[entry->rule.listnr];
	}

	list_for_each_entry(e, list, list)
		if (!audit_compare_rule(&entry->rule, &e->rule)) {
			found = e;
			goto out;
		}

out:
	return found;
}
/* …（同文件无关代码省略）… */
	struct audit_tree *tree = entry->rule.tree;
	struct list_head *list;
	int err = 0;
#ifdef CONFIG_AUDITSYSCALL
	int dont_count = 0;

	/* If any of these, don't count towards total */
	switch (entry->rule.listnr) {
	case AUDIT_FILTER_USER:
	case AUDIT_FILTER_EXCLUDE:
	case AUDIT_FILTER_FS:
		dont_count = 1;
	}
#endif

	mutex_lock(&audit_filter_mutex);
	e = audit_find_rule(entry, &list);
/* …（同文件无关代码省略）… */
			      &audit_rules_list[entry->rule.listnr]);
		list_add_tail_rcu(&entry->list, list);
	}
#ifdef CONFIG_AUDITSYSCALL
	if (!dont_count)
		audit_n_rules++;

	if (!audit_match_signal(entry))
		audit_signals++;
#endif
	mutex_unlock(&audit_filter_mutex);

	return err;
/* …（同文件无关代码省略）… */
	struct audit_entry  *e;
	struct list_head *list;
	int ret = 0;
#ifdef CONFIG_AUDITSYSCALL
	int dont_count = 0;

	/* If any of these, don't count towards total */
	switch (entry->rule.listnr) {
	case AUDIT_FILTER_USER:
	case AUDIT_FILTER_EXCLUDE:
	case AUDIT_FILTER_FS:
		dont_count = 1;
	}
#endif

	mutex_lock(&audit_filter_mutex);
	e = audit_find_rule(entry, &list);
/* …（同文件无关代码省略）… */
	if (e->rule.exe)
		audit_remove_mark_rule(&e->rule);

#ifdef CONFIG_AUDITSYSCALL
	if (!dont_count)
		audit_n_rules--;

	if (!audit_match_signal(entry))
		audit_signals--;
#endif

	call_rcu(&e->rcu, audit_free_rule_rcu);

o
