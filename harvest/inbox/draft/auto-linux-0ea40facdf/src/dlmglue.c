// AUTO-DRAFT from torvalds/linux PR #4405c18c95d2d0f99f6c581ef0ee8591a1aa6ccd
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
struct ocfs2_dlm_seq_priv {
	struct ocfs2_dlm_debug *p_dlm_debug;
	struct ocfs2_lock_res p_iter_res;
	struct ocfs2_lock_res p_tmp_res;
};
/* …（同文件无关代码省略）… */
static struct ocfs2_lock_res *ocfs2_dlm_next_res(struct ocfs2_lock_res *start,
						 struct ocfs2_dlm_seq_priv *priv)
{
	struct ocfs2_lock_res *iter, *ret = NULL;
	struct ocfs2_dlm_debug *dlm_debug = priv->p_dlm_debug;

	assert_spin_locked(&ocfs2_dlm_tracking_lock);

	list_for_each_entry(iter, &start->l_debug_list, l_debug_list) {
		/* discover the head of the list */
		if (&iter->l_debug_list == &dlm_debug->d_lockres_tracking) {
			mlog(0, "End of list found, %p\n", ret);
			break;
		}

		/* We track our "dummy" iteration lockres' by a NULL
		 * l_ops field. */
		if (iter->l_ops != NULL) {
			ret = iter;
			break;
		}
	}

	return ret;
}
/* …（同文件无关代码省略）… */
{
	struct ocfs2_dlm_seq_priv *priv = m->private;
	struct ocfs2_lock_res *iter;

	spin_lock(&ocfs2_dlm_tracking_lock);
	iter = ocfs2_dlm_next_res(&priv->p_iter_res, priv);
	if (iter) {
		/* Since lockres' have the lifetime of their container
		 * (which can be inodes, ocfs2_supers, etc) we want to
/* …（同文件无关代码省略）… */
		 * trust any pointers on the copy returned, but that's
		 * ok as the information we want isn't typically held
		 * in them. */
		priv->p_tmp_res = *iter;
		iter = &priv->p_tmp_res;
	}
	spin_unlock(&ocfs2_dlm_tracking_lock);

/* …（同文件无关代码省略）… */
static void *ocfs2_dlm_seq_next(struct seq_file *m, void *v, loff_t *pos)
{
	struct ocfs2_dlm_seq_priv *priv = m->private;
	struct ocfs2_lock_res *iter = v;
	struct ocfs2_lock_res *dummy = &priv->p_iter_res;

	(*pos)++;
	spin_lock(&ocfs2_dlm_tracking_lock);
	iter = ocfs2_dlm_next_res(iter, priv);
	list_del_init(&dummy->l_debug_list);
	if (iter) {
		list_add(&dummy->l_debug_list, &iter->l_debug_list);
		priv->p_tmp_res = *iter;
		iter = &priv->p_tmp_res;
	}
	spin_unlock(&ocfs2_dlm_tracking_lock);
