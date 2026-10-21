// AUTO-DRAFT from torvalds/linux PR #2c455ddca7827ad8fa3a64fdbf5a60efb928b46a
  // <<< BUG ANCHOR
struct file_kattr;
int cifs_fileattr_get(struct dentry *dentry, struct file_kattr *fa);
int cifs_fileattr_set(struct mnt_idmap *idmap, struct dentry *dentry,
		      struct file_kattr *fa);

