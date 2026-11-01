// AUTO-DRAFT from torvalds/linux PR #ca62fc8c6b1841039aee6de1ef03c9863751ec04
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
	};

	ovl_dentry_set_upper_alias(dentry);
	ovl_dentry_init_reval(dentry, newdentry, NULL);

	if (!hardlink) {
		/*
/* …（同文件无关代码省略）… */
		d_mark_tmpfile(tmpfile, inode);

	d_instantiate(dentry, inode);
	if (inode != oip.newinode) {
		pr_warn_ratelimited("newly created inode found in cache (%pd2)\n",
				    dentry);
/* …（同文件无关代码省略）… */
		const char *name;
		int thislen;

		spin_lock(&d->d_lock);
		name = ovl_dentry_get_redirect(d);
		if (name) {
			thislen = strlen(name);
/* …（同文件无关代码省略）… */
		if (thislen + (name[0] != '/') > buflen) {
			ret = ERR_PTR(-EXDEV);
			spin_unlock(&d->d_lock);
			goto out_put;
		}

		buflen -= thislen;
		memcpy(&buf[buflen], name, thislen);
		spin_unlock(&d->d_lock);
		tmp = dget_parent(d);

		dput(d);
		d = tmp;
