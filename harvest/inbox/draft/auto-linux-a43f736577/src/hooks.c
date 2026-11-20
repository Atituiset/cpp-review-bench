// AUTO-DRAFT from torvalds/linux PR #65e4fd942f72a9dba0e4e257290c4fa1aa756854
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static inline u32 cred_sid(const struct cred *cred)
{
	const struct cred_security_struct *crsec;

	crsec = selinux_cred(cred);
	return crsec->sid;
}
/* …（同文件无关代码省略）… */
static int inode_sid_has_perm(u32 sid, struct inode *inode, u32 perms,
			      struct common_audit_data *adp)
{
	struct inode_security_struct *isec;

	if (unlikely(IS_PRIVATE(inode)))
		return 0;

	isec = selinux_inode(inode);

	return avc_has_perm(sid, isec->sid, isec->sclass, perms, adp);
}
/* …（同文件无关代码省略）… */
static int inode_has_perm(const struct cred *cred,
			  struct inode *inode,
			  u32 perms,
			  struct common_audit_data *adp)
{
	return inode_sid_has_perm(cred_sid(cred), inode, perms, adp);
}
/* …（同文件无关代码省略）… */
static int __file_has_perm(const struct cred *cred, const struct file *file,
			   u32 av, bool bf_user_file)

{
	struct common_audit_data ad;
	struct inode *inode;
	u32 ssid = cred_sid(cred);
	u32 tsid_fd;
	int rc;

	if (bf_user_file) {
		struct backing_file_security_struct *bfsec;
		const struct path *path;

		if (WARN_ON(!(file->f_mode & FMODE_BACKING)))
			return -EIO;

		bfsec = selinux_backing_file(file);
		path = backing_file_user_path(file);
		tsid_fd = bfsec->uf_sid;
		inode = d_inode(path->dentry);

		ad.type = LSM_AUDIT_DATA_PATH;
		ad.u.path = *path;
	} else {
		struct file_security_struct *fsec = selinux_file(file);

		tsid_fd = fsec->sid;
		inode = file_inode(file);

		ad.type = LSM_AUDIT_DATA_FILE;
		ad.u.file = file;
	}

	if (ssid != tsid_fd) {
		rc = avc_has_perm(ssid, tsid_fd, SECCLASS_FD, FD__USE, &ad);
		if (rc)
			return rc;
	}

#ifdef CONFIG_BPF_SYSCALL
	/* regardless of backing vs user file, use the underlying file here */
	rc = bpf_fd_pass(file, ssid);
	if (rc)
		return rc;
#endif

	/* av is zero if only checking access to the descriptor. */
	if (av)
		return inode_has_perm(cred, inode, av, &ad);

	return 0;
}
/* …（同文件无关代码省略）… */
static u32 file_map_prot_to_av(unsigned long prot, bool shared)
{
	u32 av = FILE__READ;

	if (shared && (prot & PROT_WRITE))
		av |= FILE__WRITE;
	if (prot & PROT_EXEC)
		av |= FILE__EXECUTE;

	return av;
}
/* …（同文件无关代码省略）… */
static int __file_map_prot_check(const struct file *file, unsigned long prot,
				 bool shared, bool mounter_check,
				 bool bf_user_file)
{
	struct inode *inode = NULL;
	bool prot_exec = prot & PROT_EXEC;
	bool prot_write = prot & PROT_WRITE;

	if (file) {
		if (bf_user_file)
			inode = d_inode(backing_file_user_path(file)->dentry);
		else
			inode = file_inode(file);
	}

	if (!mounter_check && default_noexec && prot_exec &&
	    (!file || IS_PRIVATE(inode) || (!shared && prot_write))) {
		int rc;
		u32 sid = current_sid();

		/*
		 * We are making executable an anonymous mapping or a private
		 * file mapping that will also be writable.
		 */
		rc = avc_has_perm(sid, sid, SECCLASS_PROCESS, PROCESS__EXECMEM,
				  NULL);
		if (rc)
			return rc;
	}

	if (file) {
		const struct cred *cred = mounter_check ?
				file->f_cred : current_cred();

		return __file_has_perm(cred, file,
				       file_map_prot_to_av(prot, shared),
				       bf_user_file);
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static inline int file_map_prot_check(const struct file *file,
				      unsigned long prot, bool shared,
				      bool mounter_check)
{
	return __file_map_prot_check(file, prot, shared, mounter_check, false);
}
/* …（同文件无关代码省略）… */
static int selinux_mmap_file_common(struct file *file, unsigned long prot,
				    bool shared, bool mounter_check)
{
	if (file) {
		int rc;
		struct common_audit_data ad;
		const struct cred *cred = mounter_check ?
				file->f_cred : current_cred();

		ad.type = LSM_AUDIT_DATA_FILE;
		ad.u.file = file;
		rc = inode_has_perm(cred, file_inode(file), FILE__MAP, &ad);
		if (rc)
			return rc;
	}

	return file_map_prot_check(file, prot, shared, mounter_check);
}
/* …（同文件无关代码省略）… */
			     unsigned long reqprot __always_unused,
			     unsigned long prot, unsigned long flags)
{
	return selinux_mmap_file_common(file, prot,
					(flags & MAP_TYPE) == MAP_SHARED,
					false);
}

/* …（同文件无关代码省略）… */
static int bpf_fd_pass(const struct file *file, u32 sid)
{
	struct bpf_security_struct *bpfsec;
	struct bpf_prog *prog;
	struct bpf_map *map;
	int ret;

	if (file->f_op == &bpf_map_fops) {
		map = file->private_data;
		bpfsec = selinux_bpf_map_security(map);
		ret = avc_has_perm(sid, bpfsec->sid, SECCLASS_BPF,
				   bpf_map_fmode_to_av(file->f_mode), NULL);
		if (ret)
			return ret;
	} else if (file->f_op == &bpf_prog_fops) {
		prog = file->private_data;
		bpfsec = selinux_bpf_prog_security(prog);
		ret = avc_has_perm(sid, bpfsec->sid, SECCLASS_BPF,
				   BPF__PROG_RUN, NULL);
		if (ret)
			return ret;
	}
	return 0;
}
