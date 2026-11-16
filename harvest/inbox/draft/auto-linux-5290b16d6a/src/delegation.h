// AUTO-DRAFT from torvalds/linux PR #f70886e5a510cd7fc2c3aef91c46a14f4485fe63
void nfs_update_delegated_atime(struct inode *inode);
void nfs_update_delegated_mtime(struct inode *inode);
void nfs_update_delegated_mtime_locked(struct inode *inode);

static inline int nfs_have_read_or_write_delegation(struct inode *inode)
{
