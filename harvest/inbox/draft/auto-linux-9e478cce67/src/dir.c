// AUTO-DRAFT from torvalds/linux PR #f70886e5a510cd7fc2c3aef91c46a14f4485fe63
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
	spin_lock(&file->f_lock);
	desc->dir_cookie = dir_ctx->dir_cookie;
	desc->folio_index = dir_ctx->page_index;
	desc->last_cookie = dir_ctx->last_cookie;
	desc->attr_gencount = dir_ctx->attr_gencount;
	desc->eof = dir_ctx->eof;
/* …（同文件无关代码省略）… */
{
	struct nfs_server *server = NFS_SB(old_dentry->d_sb);

	if (old_dentry->d_parent != new_dentry->d_parent)
		return false;
	if (server->fh_expire_type & NFS_FH_RENAME_UNSAFE)
		return !(server->fh_expire_type & NFS_FH_NOEXPIRE_WITH_OPEN);
