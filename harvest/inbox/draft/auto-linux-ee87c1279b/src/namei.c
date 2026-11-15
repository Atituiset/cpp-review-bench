// AUTO-DRAFT from torvalds/linux PR #aacb7797504183a90d66b19b1efb555cfe9b3eec
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
struct ovl_lookup_data {
	struct super_block *sb;
	struct dentry *dentry;
	const struct ovl_layer *layer;
	struct qstr name;
	bool is_dir;
	bool opaque;
	bool xwhiteouts;
	bool stop;
	bool last;
	char *redirect;
	char *upperredirect;
	int metacopy;
	/* Referring to last redirect xattr */
	bool absolute_redirect;
};
/* …（同文件无关代码省略）… */
	char *buf;
	struct ovl_fs *ofs = OVL_FS(d->sb);

	d->absolute_redirect = false;
	buf = ovl_get_redirect_xattr(ofs, path, prelen + strlen(post));
	if (IS_ERR_OR_NULL(buf))
		return PTR_ERR(buf);

	if (buf[0] == '/') {
		d->absolute_redirect = true;
		/*
		 * One of the ancestor path elements in an absolute path
		 * lookup in ovl_lookup_layer() could have been opaque and
/* …（同文件无关代码省略）… */
out_err:
	pr_warn_ratelimited("lazy lowerdata lookup failed (%pd2, err=%i)\n",
			    dentry, err);
	goto out;
}

/* …（同文件无关代码省略）… */
static bool ovl_check_follow_redirect(struct ovl_lookup_data *d)
{
	struct ovl_fs *ofs = OVL_FS(d->sb);

	if (d->metacopy && !ofs->config.metacopy) {
		pr_warn_ratelimited("refusing to follow metacopy origin for (%pd2)\n", d->dentry);
		return false;
	}
	if ((d->redirect || d->upperredirect) && !ovl_redirect_follow(ofs)) {
		pr_warn_ratelimited("refusing to follow redirect for (%pd2)\n", d->dentry);
		return false;
	}
	return true;
}
/* …（同文件无关代码省略）… */
struct ovl_lookup_ctx {
	struct dentry *dentry;
	struct ovl_entry *oe;
	struct ovl_path *stack;
	struct ovl_path *origin_path;
	struct dentry *upperdentry;
	struct dentry *index;
	struct inode *inode;
	unsigned int ctr;
};

static int ovl_lookup_layers(struct ovl_lookup_ctx *ctx, struct ovl_lookup_data *d)
{
	struct dentry *dentry = ctx->dentry;
	struct ovl_fs *ofs = OVL_FS(dentry->d_sb);
	struct ovl_entry *poe = OVL_E(dentry->d_parent);
	struct ovl_entry *roe = OVL_E(dentry->d_sb->s_root);
	bool check_redirect = (ovl_redirect_follow(ofs) || ofs->numdatalayer);
	struct dentry *upperdir;
	struct dentry *this;
	struct dentry *origin = NULL;
/* …（同文件无关代码省略）… */
			ctx->ctr++;
		}

		if (d->stop)
			break;

/* …（同文件无关代码省略）… */
	if (d->metacopy && ctx->ctr && ofs->numdatalayer && d->absolute_redirect) {
		d->metacopy = 0;
		ctx->ctr++;
	} else if (!ovl_check_follow_redirect(d)) {
		err = -EPERM;
		return err;
/* …（同文件无关代码省略）… */
		};

		/* Store lowerdata redirect for lazy lookup */
		if (ctx->ctr > 1 && !d->is_dir && !ctx->stack[ctx->ctr - 1].dentry) {
			oip.lowerdata_redirect = d->redirect;
			d->redirect = NULL;
		}

		inode = ovl_get_inode(dentry->d_sb, &oip);
		if (IS_ERR(inode))
			return PTR_ERR(inode);

		ctx->inode = inode;
		if (ctx->upperdentry && !uppermetacopy)
			ovl_set_flag(OVL_UPPERDATA, ctx->inode);

		if (metacopy_size > OVL_METACOPY_MIN_SIZE)
			ovl_set_flag(OVL_HAS_DIGEST, ctx->inode);
	}

	ovl_dentry_init_reval(dentry, ctx->upperdentry, OVL_I_E(ctx->inode));

	return 0;
}
/* …（同文件无关代码省略）… */
{
	struct ovl_fs *ofs = OVL_FS(dentry->d_sb);
	struct ovl_entry *poe = OVL_E(dentry->d_parent);
	bool check_redirect = (ovl_redirect_follow(ofs) || ofs->numdatalayer);
	int err;
	struct ovl_lookup_ctx ctx = {
		.dentry = dentry,
/* …（同文件无关代码省略）… */
	dput(ctx.index);
	ovl_stack_free(ctx.stack, ctx.ctr);
	kfree(d.redirect);

	if (err) {
		ovl_free_entry(ctx.oe);
