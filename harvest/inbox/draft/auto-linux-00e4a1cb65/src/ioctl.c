// AUTO-DRAFT from torvalds/linux PR #2c455ddca7827ad8fa3a64fdbf5a60efb928b46a
					    compression_state);
}
  // <<< BUG ANCHOR
int cifs_fileattr_set(struct mnt_idmap *idmap, struct dentry *dentry,
		      struct file_kattr *fa)
{
	struct cifs_sb_info *cifs_sb = CIFS_SB(dentry);
