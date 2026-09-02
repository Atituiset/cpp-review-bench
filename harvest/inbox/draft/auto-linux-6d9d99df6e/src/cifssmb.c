// AUTO-DRAFT from torvalds/linux PR #89a312991dc6e638a36adc43ccb91dbc25504c04
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static void cifs_init_ace(struct cifs_posix_ace *cifs_ace,
			  const struct posix_acl_entry *local_ace)
{
	cifs_ace->cifs_e_perm = local_ace->e_perm;
	cifs_ace->cifs_e_tag =  local_ace->e_tag;

	switch (local_ace->e_tag) {
	case ACL_USER:
		cifs_ace->cifs_uid =
			cpu_to_le64(from_kuid(&init_user_ns, local_ace->e_uid));
		break;
	case ACL_GROUP:
		cifs_ace->cifs_uid =
			cpu_to_le64(from_kgid(&init_user_ns, local_ace->e_gid));
		break;
	default:
		cifs_ace->cifs_uid = cpu_to_le64(-1);
	}
}
/* …（同文件无关代码省略）… */
static __u16 posix_acl_to_cifs(char *parm_data, const struct posix_acl *acl,
			       const int acl_type)
{
	__u16 rc = 0;
	struct cifs_posix_acl *cifs_acl = (struct cifs_posix_acl *)parm_data;
	const struct posix_acl_entry *pa, *pe;
	int count;
	int i = 0;

	if ((acl == NULL) || (cifs_acl == NULL))
		return 0;

	count = acl->a_count;
	cifs_dbg(FYI, "setting acl with %d entries\n", count);

	/*
	 * Note that the uapi POSIX ACL version is verified by the VFS and is
	 * independent of the cifs ACL version. Changing the POSIX ACL version
	 * is a uapi change and if it's changed we will pass down the POSIX ACL
	 * version in struct posix_acl from the VFS. For now there's really
	 * only one that all filesystems know how to deal with.
	 */
	cifs_acl->version = cpu_to_le16(1);
	if (acl_type == ACL_TYPE_ACCESS) {
		cifs_acl->access_entry_count = cpu_to_le16(count);
		cifs_acl->default_entry_count = cpu_to_le16(0xFFFF);
	} else if (acl_type == ACL_TYPE_DEFAULT) {
		cifs_acl->default_entry_count = cpu_to_le16(count);
		cifs_acl->access_entry_count = cpu_to_le16(0xFFFF);
	} else {
		cifs_dbg(FYI, "unknown ACL type %d\n", acl_type);
		return 0;
	}
	FOREACH_ACL_ENTRY(pa, acl, pe) {
		cifs_init_ace(&cifs_acl->ace_array[i++], pa);
	}
	if (rc == 0) {
		rc = (__u16)(count * sizeof(struct cifs_posix_ace));
		rc += sizeof(struct cifs_posix_acl);
		/* BB add check to make sure ACL does not overflow SMB */
	}
	return rc;
}
/* …（同文件无关代码省略）… */
	int rc = 0;
	int bytes_returned = 0;
	__u16 params, byte_count, data_count, param_offset, offset;

	cifs_dbg(FYI, "In SetPosixACL (Unix) for path %s\n", fileName);
setAclRetry:
/* …（同文件无关代码省略）… */
	}
	params = 6 + name_len;
	pSMB->MaxParameterCount = cpu_to_le16(2);
	/* BB find max SMB size from sess */
	pSMB->MaxDataCount = cpu_to_le16(1000);
	pSMB->MaxSetupCount = 0;
	pSMB->Reserved = 0;
	pSMB->Flags = 0;
/* …（同文件无关代码省略）… */
	parm_data = ((char *)pSMB) + offset;
	pSMB->ParameterOffset = cpu_to_le16(param_offset);

	/* convert to on the wire format for POSIX ACL */
	data_count = posix_acl_to_cifs(parm_data, acl, acl_type);

/* …（同文件无关代码省略）… */
	int name_len;
	int rc = 0;
	int bytes_returned = 0;
	__u16 params, param_offset, byte_count, offset, count;
	int remap = cifs_remap(cifs_sb);

	cifs_dbg(FYI, "In SetEA\n");
SetEARetry:
/* …（同文件无关代码省略）… */
	pSMB->Reserved3 = 0;
	pSMB->SubCommand = cpu_to_le16(TRANS2_SET_PATH_INFORMATION);
	byte_count = 3 /* pad */  + params + count;
	pSMB->DataCount = cpu_to_le16(count);
	parm_data->list_len = cpu_to_le32(count);
	parm_data->list.EA_flags = 0;
