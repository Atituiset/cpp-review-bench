// AUTO-DRAFT from torvalds/linux PR #cba88960471ac67635080708b67ed5c2a2e4e9fd
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
 */

#include <linux/fs.h>
#include <kunit/visibility.h>
#include <linux/slab.h>
#include <linux/string.h>
/* …（同文件无关代码省略）… */
int compare_sids(const struct smb_sid *ctsid, const struct smb_sid *cwsid)
{
	int i;
	int num_subauth, num_sat, num_saw;

	if (!ctsid || !cwsid)
		return 1;

	/* compare the revision */
	if (ctsid->revision != cwsid->revision) {
		if (ctsid->revision > cwsid->revision)
			return 1;
		else
			return -1;
	}

	/* compare all of the six auth values */
	for (i = 0; i < NUM_AUTHS; ++i) {
		if (ctsid->authority[i] != cwsid->authority[i]) {
			if (ctsid->authority[i] > cwsid->authority[i])
				return 1;
			else
				return -1;
		}
	}

	/* compare all of the subauth values if any */
	num_sat = ctsid->num_subauth;
	num_saw = cwsid->num_subauth;
	num_subauth = min(num_sat, num_saw);
	if (num_subauth) {
		for (i = 0; i < num_subauth; ++i) {
			if (ctsid->sub_auth[i] != cwsid->sub_auth[i]) {
				if (le32_to_cpu(ctsid->sub_auth[i]) >
				    le32_to_cpu(cwsid->sub_auth[i]))
					return 1;
				else
					return -1;
			}
		}
	}

	return 0; /* sids compare/match */
}
/* …（同文件无关代码省略）… */
static umode_t access_flags_to_mode(struct smb_fattr *fattr, __le32 ace_flags,
				    int type)
{
	__u32 flags = le32_to_cpu(ace_flags);
	umode_t mode = 0;

	if (flags & GENERIC_ALL) {
		mode = 0777;
		ksmbd_debug(SMB, "all perms\n");
		return mode;
	}

	if ((flags & GENERIC_READ) || (flags & FILE_READ_RIGHTS))
		mode = 0444;
	if ((flags & GENERIC_WRITE) || (flags & FILE_WRITE_RIGHTS)) {
		mode |= 0222;
		if (S_ISDIR(fattr->cf_mode))
			mode |= 0111;
	}
	if ((flags & GENERIC_EXECUTE) || (flags & FILE_EXEC_RIGHTS))
		mode |= 0111;

	if (type == ACCESS_DENIED_ACE_TYPE || type == ACCESS_DENIED_OBJECT_ACE_TYPE)
		mode = ~mode;

	ksmbd_debug(SMB, "access flags 0x%x mode now %04o\n", flags, mode);

	return mode;
}
/* …（同文件无关代码省略）… */
static int sid_to_id(const struct mnt_idmap *idmap,
		     struct smb_sid *psid, uint sidtype,
		     struct smb_fattr *fattr)
{
	const struct smb_sid *sid_prefix;
	int rc = -EINVAL;

	/*
	 * If we have too many subauthorities, then something is really wrong.
	 * Just return an error.
	 */
	if (unlikely(psid->num_subauth > SID_MAX_SUB_AUTHORITIES)) {
		pr_err("%s: %u subauthorities is too many!\n",
		       __func__, psid->num_subauth);
		return -EIO;
	}

	if (psid->num_subauth == 0) {
		pr_err("%s: zero subauthorities!\n", __func__);
		return -EIO;
	}

	if (sidtype == SIDOWNER) {
		kuid_t uid;
		uid_t id;

		/* Only the server domain RID has a local uid representation. */
		sid_prefix = &server_conf.domain_sid;
		if (psid->num_subauth != sid_prefix->num_subauth + 1 ||
		    compare_sids(psid, sid_prefix))
			return -EINVAL;

		id = le32_to_cpu(psid->sub_auth[psid->num_subauth - 1]);
		uid = KUIDT_INIT(id);
		uid = from_vfsuid(idmap, &init_user_ns, VFSUIDT_INIT(uid));
		if (uid_valid(uid)) {
			fattr->cf_uid = uid;
			rc = 0;
		}
	} else {
		kgid_t gid;
		gid_t id;

		/* Local gids are represented by S-1-22-2-<gid>. */
		sid_prefix = &sid_unix_groups;
		if (psid->num_subauth != sid_prefix->num_subauth + 1 ||
		    compare_sids(psid, sid_prefix))
			return -EINVAL;

		id = le32_to_cpu(psid->sub_auth[psid->num_subauth - 1]);
		gid = KGIDT_INIT(id);
		gid = from_vfsgid(idmap, &init_user_ns, VFSGIDT_INIT(gid));
		if (gid_valid(gid)) {
			fattr->cf_gid = gid;
			rc = 0;
		}
	}

	return rc;
}
/* …（同文件无关代码省略）… */
void posix_state_to_acl(struct posix_acl_state *state,
			struct posix_acl_entry *pace)
{
	int i;

	pace->e_tag = ACL_USER_OBJ;
	pace->e_perm = state->owner.allow;
	for (i = 0; i < state->users->n; i++) {
		pace++;
		pace->e_tag = ACL_USER;
		pace->e_uid = state->users->aces[i].uid;
		pace->e_perm = state->users->aces[i].perms.allow;
	}

	pace++;
	pace->e_tag = ACL_GROUP_OBJ;
	pace->e_perm = state->group.allow;

	for (i = 0; i < state->groups->n; i++) {
		pace++;
		pace->e_tag = ACL_GROUP;
		pace->e_gid = state->groups->aces[i].gid;
		pace->e_perm = state->groups->aces[i].perms.allow;
	}

	if (state->users->n || state->groups->n) {
		pace++;
		pace->e_tag = ACL_MASK;
		pace->e_perm = state->mask.allow;
	}

	pace++;
	pace->e_tag = ACL_OTHER;
	pace->e_perm = state->other.allow;
}
/* …（同文件无关代码省略）… */
int init_acl_state(struct posix_acl_state *state, u16 cnt)
{
	int alloc;

	memset(state, 0, sizeof(struct posix_acl_state));
	/*
	 * In the worst case, each individual acl could be for a distinct
	 * named user or group, but we don't know which, so we allocate
	 * enough space for either:
	 */
	alloc = sizeof(struct posix_ace_state_array)
		+ cnt * sizeof(struct posix_user_ace_state);
	state->users = kzalloc(alloc, KSMBD_DEFAULT_GFP);
	if (!state->users)
		return -ENOMEM;
	state->groups = kzalloc(alloc, KSMBD_DEFAULT_GFP);
	if (!state->groups) {
		kfree(state->users);
		return -ENOMEM;
	}
	return 0;
}
/* …（同文件无关代码省略）… */
void free_acl_state(struct posix_acl_state *state)
{
	kfree(state->users);
	kfree(state->groups);
}
/* …（同文件无关代码省略）… */
static int parse_dacl(const struct mnt_idmap *idmap,
		      struct smb_acl *pdacl, char *end_of_acl,
		      struct smb_sid *pownersid, struct smb_sid *pgrpsid,
		      struct smb_fattr *fattr)
{
	int i, ret;
	u16 num_aces = 0;
	u16 dacl_size;
	unsigned int acl_size;
	char *acl_base;
	struct smb_ace **ppace;
	struct posix_acl_entry *cf_pace, *cf_pdace;
	struct posix_acl_state acl_state, default_acl_state;
	umode_t mode = 0, acl_mode;
	bool owner_found = false, group_found = false, others_found = false;

	if (!pdacl)
		return 0;

	/* validate that we do not go past end of acl */
	if (end_of_acl < (char *)pdacl + sizeof(struct smb_acl) ||
	    end_of_acl < (char *)pdacl + le16_to_cpu(pdacl->size)) {
		pr_err("ACL too small to parse DACL\n");
		return -EINVAL;
	}

	ksmbd_debug(SMB, "DACL revision %d size %d num aces %d\n",
		    le16_to_cpu(pdacl->revision), le16_to_cpu(pdacl->size),
		    le16_to_cpu(pdacl->num_aces));

	acl_base = (char *)pdacl;
	acl_size = sizeof(struct smb_acl);

	num_aces = le16_to_cpu(pdacl->num_aces);
	if (num_aces <= 0)
		return 0;

	dacl_size = le16_to_cpu(pdacl->size);
	if (dacl_size < sizeof(struct smb_acl))
		return -EINVAL;

	if (num_aces > (dacl_size - sizeof(struct smb_acl)) /
			(offsetof(struct smb_ace, sid) +
			 offsetof(struct smb_sid, sub_auth) + sizeof(__le16)))
		return -EINVAL;

	ret = init_acl_state(&acl_state, num_aces);
	if (ret)
		return ret;
	ret = init_acl_state(&default_acl_state, num_aces);
	if (ret) {
		free_acl_state(&acl_state);
		return ret;
	}

	ppace = kmalloc_objs(struct smb_ace *, num_aces, KSMBD_DEFAULT_GFP);
	if (!ppace) {
		free_acl_state(&default_acl_state);
		free_acl_state(&acl_state);
		return -ENOMEM;
	}

	/*
	 * reset rwx permissions for user/group/other.
	 * Also, if num_aces is 0 i.e. DACL has no ACEs,
	 * user/group/other have no permissions
	 */
	for (i = 0; i < num_aces; ++i) {
		if (end_of_acl - acl_base < acl_size) {
			ret = -EINVAL;
			goto out;
		}

		ppace[i] = (struct smb_ace *)(acl_base + acl_size);
		acl_base = (char *)ppace[i];
		acl_size = offsetof(struct smb_ace, sid) +
			offsetof(struct smb_sid, sub_auth);

		if (end_of_acl - acl_base < acl_size ||
		    ppace[i]->sid.num_subauth == 0 ||
		    ppace[i]->sid.num_subauth > SID_MAX_SUB_AUTHORITIES ||
		    (end_of_acl - acl_base <
		     acl_size + sizeof(__le32) * ppace[i]->sid.num_subauth) ||
		    (le16_to_cpu(ppace[i]->size) <
		     acl_size + sizeof(__le32) * ppace[i]->sid.num_subauth)) {
			ret = -EINVAL;
			goto out;
		}

		acl_size = le16_to_cpu(ppace[i]->size);
		ppace[i]->access_req =
			smb_map_generic_desired_access(ppace[i]->access_req);

		if (ppace[i]->type == ACCESS_ALLOWED &&
		    ppace[i]->sid.num_subauth >= 3 &&
		    !(compare_sids(&ppace[i]->sid, &sid_unix_NFS_mode))) {
			fattr->cf_mode =
				le32_to_cpu(ppace[i]->sid.sub_auth[2]);
			fattr->cf_posix_mode = true;
			break;
		} else if (!compare_sids(&ppace[i]->sid, pownersid)) {
			acl_mode = access_flags_to_mode(fattr,
							ppace[i]->access_req,
							ppace[i]->type);
			acl_mode &= 0700;

			if (!owner_found) {
				mode &= ~(0700);
				mode |= acl_mode;
			}
			owner_found = true;
		} else if (!compare_sids(&ppace[i]->sid, pgrpsid) ||
			   ppace[i]->sid.sub_auth[ppace[i]->sid.num_subauth - 1] ==
			    DOMAIN_USER_RID_LE) {
			acl_mode = access_flags_to_mode(fattr,
							ppace[i]->access_req,
							ppace[i]->type);
			acl_mode &= 0070;
			if (!group_found) {
				mode &= ~(0070);
				mode |= acl_mode;
			}
			group_found = true;
		} else if (!compare_sids(&ppace[i]->sid, &sid_everyone)) {
			acl_mode = access_flags_to_mode(fattr,
							ppace[i]->access_req,
							ppace[i]->type);
			acl_mode &= 0007;
			if (!others_found) {
				mode &= ~(0007);
				mode |= acl_mode;
			}
			others_found = true;
		} else if (!compare_sids(&ppace[i]->sid, &creator_owner)) {
			continue;
		} else if (!compare_sids(&ppace[i]->sid, &creator_group)) {
			continue;
		} else if (!compare_sids(&ppace[i]->sid, &sid_authusers)) {
			continue;
		} else {
			struct smb_fattr temp_fattr;

			acl_mode = access_flags_to_mode(fattr, ppace[i]->access_req,
							ppace[i]->type);
			temp_fattr.cf_uid = INVALID_UID;
			ret = sid_to_id(idmap, &ppace[i]->sid, SIDOWNER, &temp_fattr);
			if (ret || uid_eq(temp_fattr.cf_uid, INVALID_UID)) {
				pr_err_ratelimited("%s: Error %d mapping Owner SID to uid\n",
						   __func__, ret);
				continue;
			}

			acl_state.owner.allow = ((acl_mode & 0700) >> 6) | 0004;
			acl_state.users->aces[acl_state.users->n].uid =
				temp_fattr.cf_uid;
			acl_state.users->aces[acl_state.users->n++].perms.allow =
				((acl_mode & 0700) >> 6) | 0004;
			default_acl_state.owner.allow = ((acl_mode & 0700) >> 6) | 0004;
			default_acl_state.users->aces[default_acl_state.users->n].uid =
				temp_fattr.cf_uid;
			default_acl_state.users->aces[default_acl_state.users->n++].perms.allow =
				((acl_mode & 0700) >> 6) | 0004;
		}
	}
	if (fattr->cf_posix_mode) {
		ret = 0;
		goto out;
	}

	if (owner_found) {
		/* The owner must be set to at least read-only. */
		acl_state.owner.allow = ((mode & 0700) >> 6) | 0004;
		acl_state.users->aces[acl_state.users->n].uid = fattr->cf_uid;
		acl_state.users->aces[acl_state.users->n++].perms.allow =
			((mode & 0700) >> 6) | 0004;
		default_acl_state.owner.allow = ((mode & 0700) >> 6) | 0004;
		default_acl_state.users->aces[default_acl_state.users->n].uid =
			fattr->cf_uid;
		default_acl_state.users->aces[default_acl_state.users->n++].perms.allow =
			((mode & 0700) >> 6) | 0004;
	}

	if (group_found) {
		acl_state.group.allow = (mode & 0070) >> 3;
		acl_state.groups->aces[acl_state.groups->n].gid =
			fattr->cf_gid;
		acl_state.groups->aces[acl_state.groups->n++].perms.allow =
			(mode & 0070) >> 3;
		default_acl_state.group.allow = (mode & 0070) >> 3;
		default_acl_state.groups->aces[default_acl_state.groups->n].gid =
			fattr->cf_gid;
		default_acl_state.groups->aces[default_acl_state.groups->n++].perms.allow =
			(mode & 0070) >> 3;
	}

	if (others_found) {
		fattr->cf_mode &= ~(0007);
		fattr->cf_mode |= mode & 0007;

		acl_state.other.allow = mode & 0007;
		default_acl_state.other.allow = mode & 0007;
	}

	if (acl_state.users->n || acl_state.groups->n) {
		acl_state.mask.allow = 0x07;

		if (IS_ENABLED(CONFIG_FS_POSIX_ACL)) {
			fattr->cf_acls =
				posix_acl_alloc(acl_state.users->n +
					acl_state.groups->n + 4, KSMBD_DEFAULT_GFP);
			if (!fattr->cf_acls) {
				ret = -ENOMEM;
				goto out;
			}
			cf_pace = fattr->cf_acls->a_entries;
			posix_state_to_acl(&acl_state, cf_pace);
		}
	}

	if (default_acl_state.users->n || default_acl_state.groups->n) {
		default_acl_state.mask.allow = 0x07;

		if (IS_ENABLED(CONFIG_FS_POSIX_ACL)) {
			fattr->cf_dacls =
				posix_acl_alloc(default_acl_state.users->n +
				default_acl_state.groups->n + 4, KSMBD_DEFAULT_GFP);
			if (!fattr->cf_dacls) {
				ret = -ENOMEM;
				goto out;
			}
			cf_pdace = fattr->cf_dacls->a_entries;
			posix_state_to_acl(&default_acl_state, cf_pdace);
		}
	}
	ret = 0;
out:
	kfree(ppace);
	free_acl_state(&acl_state);
	free_acl_state(&default_acl_state);
	return ret;
}
/* …（同文件无关代码省略）… */
static int parse_sid(struct smb_sid *psid, char *end_of_acl)
{
	/*
	 * validate that we do not go past end of ACL - sid must be at least 8
	 * bytes long (assuming no sub-auths - e.g. the null SID
	 */
	if (end_of_acl < (char *)psid + 8) {
		pr_err("ACL too small to parse SID %p\n", psid);
		return -EINVAL;
	}

	if (!psid->num_subauth)
		return 0;

	if (psid->num_subauth > SID_MAX_SUB_AUTHORITIES ||
	    end_of_acl < (char *)psid + 8 + sizeof(__le32) * psid->num_subauth)
		return -EINVAL;

	return 0;
}
/* …（同文件无关代码省略）… */
int parse_sec_desc(const struct mnt_idmap *idmap, struct smb_ntsd *pntsd,
		   int acl_len, struct smb_fattr *fattr)
{
	int rc = 0;
	struct smb_sid *owner_sid_ptr = NULL, *group_sid_ptr = NULL;
	struct smb_acl *dacl_ptr; /* no need for SACL ptr */
	char *end_of_acl = ((char *)pntsd) + acl_len;
	__u32 dacloffset;
	int pntsd_type;

	if (!pntsd)
		return -EIO;

	if (acl_len < sizeof(struct smb_ntsd))
		return -EINVAL;

	dacloffset = le32_to_cpu(pntsd->dacloffset);
	ksmbd_debug(SMB,
		    "revision %d type 0x%x ooffset 0x%x goffset 0x%x sacloffset 0x%x dacloffset 0x%x\n",
		    pntsd->revision, pntsd->type, le32_to_cpu(pntsd->osidoffset),
		    le32_to_cpu(pntsd->gsidoffset),
		    le32_to_cpu(pntsd->sacloffset), dacloffset);

	pntsd_type = le16_to_cpu(pntsd->type);
	if (!(pntsd_type & DACL_PRESENT)) {
		ksmbd_debug(SMB, "DACL_PRESENT in DACL type is not set\n");
		return rc;
	}

	pntsd->type = cpu_to_le16(DACL_PRESENT);

	if (pntsd->osidoffset) {
		if (le32_to_cpu(pntsd->osidoffset) < sizeof(struct smb_ntsd) ||
		    le32_to_cpu(pntsd->osidoffset) >= acl_len)
			return -EINVAL;

		owner_sid_ptr = (struct smb_sid *)((char *)pntsd +
				le32_to_cpu(pntsd->osidoffset));
		rc = parse_sid(owner_sid_ptr, end_of_acl);
		if (rc) {
			pr_err("%s: Error %d parsing Owner SID\n", __func__, rc);
			return rc;
		}

		rc = sid_to_id(idmap, owner_sid_ptr, SIDOWNER, fattr);
		if (rc) {
			ksmbd_debug(SMB, "Owner SID has no Unix uid mapping\n");
			owner_sid_ptr = NULL;
			rc = 0;
		}
	}

	if (pntsd->gsidoffset) {
		if (le32_to_cpu(pntsd->gsidoffset) < sizeof(struct smb_ntsd) ||
		    le32_to_cpu(pntsd->gsidoffset) >= acl_len)
			return -EINVAL;

		group_sid_ptr = (struct smb_sid *)((char *)pntsd +
				le32_to_cpu(pntsd->gsidoffset));
		rc = parse_sid(group_sid_ptr, end_of_acl);
		if (rc) {
			pr_err("%s: Error %d mapping Owner SID to gid\n",
			       __func__, rc);
			return rc;
		}
		rc = sid_to_id(idmap, group_sid_ptr, SIDUNIX_GROUP, fattr);
		if (rc) {
			ksmbd_debug(SMB, "Group SID has no Unix gid mapping\n");
			group_sid_ptr = NULL;
			rc = 0;
		}
	}

	if ((pntsd_type & (DACL_AUTO_INHERITED | DACL_AUTO_INHERIT_REQ)) ==
	    (DACL_AUTO_INHERITED | DACL_AUTO_INHERIT_REQ))
		pntsd->type |= cpu_to_le16(DACL_AUTO_INHERITED);
	if (pntsd_type & DACL_PROTECTED)
		pntsd->type |= cpu_to_le16(DACL_PROTECTED);

	if (dacloffset) {
		if (dacloffset < sizeof(struct smb_ntsd) || dacloffset >= acl_len)
			return -EINVAL;

		dacl_ptr = (struct smb_acl *)((char *)pntsd + dacloffset);
		rc = parse_dacl(idmap, dacl_ptr, end_of_acl,
				owner_sid_ptr, group_sid_ptr, fattr);
		if (rc)
			return rc;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
	struct smb_acl *parent_pdacl;
	struct smb_ntsd *parent_pntsd = NULL;
	struct smb_sid owner_sid, group_sid;
	struct dentry *parent = path->dentry->d_parent;
	const struct mnt_idmap *idmap = mnt_idmap(path->mnt);
	int inherited_flags = 0, flags = 0, i, nt_size = 0, pdacl_size;
	int rc = 0, pntsd_type, ppntsd_size, acl_len, aces_size;
/* …（同文件无关代码省略）… */
	char *aces_base;
	bool is_dir = S_ISDIR(d_inode(path->dentry)->i_mode);

	ppntsd_size = ksmbd_vfs_get_sd_xattr(conn, idmap,
					    parent, &parent_pntsd);
	if (ppntsd_size <= 0)
		return -ENOENT;

/* …（同文件无关代码省略）… */
	return false;
}

int smb_check_perm_dacl(struct ksmbd_conn *conn, const struct path *path,
			__le32 *pdaccess, __le32 raw_daccess, int uid,
			bool strict)
{
	const struct mnt_idmap *idmap = mnt_idmap(path->mnt);
	struct smb_ntsd *pntsd = NULL;
/* …（同文件无关代码省略）… */
	int granted = requested;
	struct smb_ace *ace;
	int i, found = 0;
	unsigned int access_bits = 0, denied = 0;
	struct smb_ace *others_ace = NULL;
	bool or_ace = false;
	struct posix_acl_entry *pa_entry;
	unsigned int sid_type = SIDOWNER;
	unsigned short ace_size;
	bool is_owner, owner_rights = false;
	vfsuid_t vfsuid;

	ksmbd_debug(SMB, "check permission using windows acl\n");
/* …（同文件无关代码省略）… */
		/*
		 * An empty (present, zero-ACE) DACL grants no access to
		 * anyone except the object owner's implicit READ_CONTROL
		 * and WRITE_DAC (MS-DTYP 2.4.5). Deny every other caller,
		 * deny the owner any access beyond those two bits, and do
		 * not let trailing bytes after a zero-ACE DACL become an
		 * implicit grant.
		 */
		if (is_owner)
			allowed = READ_CONTROL | WRITE_DAC;
		if (*pdaccess & FILE_MAXIMAL_ACCESS_LE)
			requested |= allowed;
		if (!allowed || requested & ~allowed)
			rc = -EACCES;
/* …（同文件无关代码省略）… */
		goto err_out;
	}

	if (*pdaccess & FILE_MAXIMAL_ACCESS_LE) {
		ace = (struct smb_ace *)((char *)pdacl + sizeof(struct smb_acl));
		aces_size = pdacl_size - sizeof(struct smb_acl);
		for (i = 0; i < le16_to_cpu(pdacl->num_aces); i++) {
			if (aces_size < offsetof(struct smb_ace, sid) +
			    CIFS_SID_BASE_SIZE)
				break;
			ace_size = le16_to_cpu(ace->size);
			if (ace_size > aces_size ||
			    ace_size < offsetof(struct smb_ace, sid) +
				       CIFS_SID_BASE_SIZE)
				break;
			aces_size -= ace_size;

			if (ace->sid.num_subauth > SID_MAX_SUB_AUTHORITIES ||
			    ace_size < offsetof(struct smb_ace, sid) +
				      CIFS_SID_BASE_SIZE +
				      sizeof(__le32) * ace->sid.num_subauth)
				break;

			if (!compare_sids(&sid_owner_rights, &ace->sid)) {
				owner_rights = true;
				if (!is_owner)
					goto next_ace;
			}

			if (ace->flags & INHERIT_ONLY_ACE ||
			    (compare_sids(&sid, &ace->sid) &&
			     compare_sids(&sid_unix_NFS_mode, &ace->sid) &&
			     compare_sids(&sid_everyone, &ace->sid) &&
			     compare_sids(&sid_authusers, &ace->sid) &&
			     compare_sids(&sid_owner_rights, &ace->sid)))
				goto next_ace;

			switch (ace->type) {
			case ACCESS_ALLOWED_ACE_TYPE:
				access_bits |= le32_to_cpu(ace->access_req);
				break;
			case ACCESS_DENIED_ACE_TYPE:
			case ACCESS_DENIED_CALLBACK_ACE_TYPE:
				denied |= ~access_bits &
					le32_to_cpu(ace->access_req);
				break;
			}
next_ace:
			ace = (struct smb_ace *)((char *)ace + le16_to_cpu(ace->size));
		}
		if (is_owner && !owner_rights)
			access_bits |= READ_CONTROL | WRITE_DAC |
				FILE_READ_ATTRIBUTES | DELETE;
		access_bits &= ~denied;
		if ((raw_daccess & FILE_GENERIC_EXECUTE_LE) &&
		    S_ISREG(d_inode(path->dentry)->i_mode) &&
		    (access_bits & GENERIC_READ_FLAGS) == GENERIC_READ_FLAGS)
			access_bits |= FILE_EXECUTE;
		granted = requested | access_bits;
	}

	ace = (struct smb_ace *)((char *)pdacl + sizeof(struct smb_acl));
/* …（同文件无关代码省略）… */
		    !(ace->flags & INHERIT_ONLY_ACE))
			or_ace = true;
		if (!compare_sids(&sid_everyone, &ace->sid) ||
		    !compare_sids(&sid_authusers, &ace->sid))
			others_ace = ace;

		ace = (struct smb_ace *)((char *)ace + le16_to_cpu(ace->size));
	}

	if (IS_ENABLED(CONFIG_FS_POSIX_ACL)) {
		posix_acls = get_inode_acl(d_inode(path->dentry), ACL_TYPE_ACCESS);
		if (!IS_ERR_OR_NULL(posix_acls) && !found && !others_ace) {
			unsigned int id = -1;

			pa_entry = posix_acls->a_entries;
/* …（同文件无关代码省略）… */
					if (!access_bits)
						access_bits =
							SET_MINIMUM_RIGHTS;
					posix_acl_release(posix_acls);
					goto check_access_bits;
				}
/* …（同文件无关代码省略）… */
			 */
			if (is_owner && !or_ace)
				allowed = READ_CONTROL | WRITE_DAC;
			if (*pdaccess & FILE_MAXIMAL_ACCESS_LE)
				requested |= allowed;
			if (!allowed || requested & ~allowed) {
				ksmbd_debug(SMB, "Can't find corresponding sid\n");
/* …（同文件无关代码省略）… */
		}
	}

	if (!(*pdaccess & FILE_MAXIMAL_ACCESS_LE)) {
		switch (ace->type) {
		case ACCESS_ALLOWED_ACE_TYPE:
			access_bits = le32_to_cpu(ace->access_req);
/* …（同文件无关代码省略）… */
}
EXPORT_SYMBOL_IF_KUNIT(smb_check_perm_dacl);

int set_info_sec(struct ksmbd_conn *conn, struct ksmbd_tree_connect *tcon,
		 const struct path *path, struct smb_ntsd *pntsd, int ntsd_len,
		 bool type_check, bool get_write, bool posix_open,
		 bool dacl_requested)
{
	int rc;
	struct smb_fattr fattr = {{0}};
	struct inode *inode = d_inode(path->dentry);
	const struct mnt_idmap *idmap = mnt_idmap(path->mnt);
	struct iattr newattrs;

	fattr.cf_uid = INVALID_UID;
	fattr.cf_gid = INVALID_GID;
	fattr.cf_mode = inode->i_mode;

	rc = parse_sec_desc(idmap, pntsd, ntsd_len, &fattr);
	if (rc)
		goto out;
	if (fattr.cf_posix_mode && !dacl_requested) {
		fattr.cf_posix_mode = false;
		fattr.cf_mode = inode->i_mode;
	}
	if (fattr.cf_posix_mode && !posix_open) {
		rc = -EOPNOTSUPP;
		goto out;
	}

	newattrs.ia_valid = ATTR_CTIME;
	if (!uid_eq(fattr.cf_uid, INVALID_UID)) {
		newattrs.ia_valid |= ATTR_UID;
		newattrs.ia_uid = fattr.cf_uid;
	}
	if (!gid_eq(fattr.cf_gid, INVALID_GID)) {
		newattrs.ia_valid |= ATTR_GID;
		newattrs.ia_gid = fattr.cf_gid;
	}
	newattrs.ia_valid |= ATTR_MODE;
	if (fattr.cf_posix_mode)
		newattrs.ia_mode = (inode->i_mode & ~07777) |
				   (fattr.cf_mode & 07777);
	else
		newattrs.ia_mode = (inode->i_mode & ~0777) |
				   (fattr.cf_mode & 0777);

	ksmbd_vfs_remove_acl_xattrs(idmap, path);
	/* Update posix acls */
	if (IS_ENABLED(CONFIG_FS_POSIX_ACL) && fattr.cf_dacls) {
		rc = set_posix_acl(idmap, path->dentry,
				   ACL_TYPE_ACCESS, fattr.cf_acls);
		if (rc < 0)
			ksmbd_debug(SMB,
				    "Set posix acl(ACL_TYPE_ACCESS) failed, rc : %d\n",
				    rc);
		if (S_ISDIR(inode->i_mode) && fattr.cf_dacls) {
			rc = set_posix_acl(idmap, path->dentry,
					   ACL_TYPE_DEFAULT, fattr.cf_dacls);
			if (rc)
				ksmbd_debug(SMB,
					    "Set posix acl(ACL_TYPE_DEFAULT) failed, rc : %d\n",
					    rc);
		}
	}

	inode_lock(inode);
	rc = notify_change(idmap, path->dentry, &newattrs, NULL);
	inode_unlock(inode);
	if (rc)
		goto out;

	/* Check it only calling from SD BUFFER context */
	if (type_check && !(le16_to_cpu(pntsd->type) & DACL_PRESENT))
		goto out;

	if (test_share_config_flag(tcon->share_conf, KSMBD_SHARE_FLAG_ACL_XATTR)) {
		/* Update WinACL in xattr */
		ksmbd_vfs_remove_sd_xattrs(idmap, path);
		ksmbd_vfs_set_sd_xattr(conn, idmap, path, pntsd, ntsd_len,
				get_write);
	}

out:
	posix_acl_release(fattr.cf_acls);
	posix_acl_release(fattr.cf_dacls);
	return rc;
}
