// AUTO-DRAFT from torvalds/linux PR #7f75cd8ccd9588e969158c5dc92a8cc217663f5b
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
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
static void mode_to_access_flags(umode_t mode, umode_t bits_to_use,
				 __u32 *pace_flags)
{
	/* reset access mask */
	*pace_flags = 0x0;

	/* bits to use are either S_IRWXU or S_IRWXG or S_IRWXO */
	mode &= bits_to_use;

	/*
	 * check for R/W/X UGO since we do not know whose flags
	 * is this but we have cleared all the bits sans RWX for
	 * either user or group or other as per bits_to_use
	 */
	if (mode & 0444)
		*pace_flags |= SET_FILE_READ_RIGHTS;
	if (mode & 0222)
		*pace_flags |= FILE_WRITE_RIGHTS;
	if (mode & 0111)
		*pace_flags |= SET_FILE_EXEC_RIGHTS;

	ksmbd_debug(SMB, "mode: %o, access flags now 0x%x\n",
		    mode, *pace_flags);
}
/* …（同文件无关代码省略）… */
	int i, found = 0;
	unsigned int access_bits = 0, denied = 0;
	struct smb_ace *others_ace = NULL;
	struct posix_acl_entry *pa_entry;
	unsigned int sid_type = SIDOWNER;
	unsigned short ace_size;
/* …（同文件无关代码省略）… */
	is_owner = uid == from_kuid(&init_user_ns, vfsuid_into_kuid(vfsuid));

	if (!pdacl->num_aces) {
		/*
		 * An empty (present, zero-ACE) DACL grants no access to
		 * anyone except the object owner's implicit READ_CONTROL
/* …（同文件无关代码省略）… */
		 * not let trailing bytes after a zero-ACE DACL become an
		 * implicit grant.
		 */
		if (!is_owner ||
		    (*pdaccess & ~(FILE_READ_CONTROL_LE | FILE_WRITE_DAC_LE)))
			rc = -EACCES;
		goto err_out;
	}

/* …（同文件无关代码省略）… */
			found = 1;
			break;
		}
		if (!compare_sids(&sid_everyone, &ace->sid) ||
		    !compare_sids(&sid_authusers, &ace->sid))
			others_ace = ace;
/* …（同文件无关代码省略）… */
				else
					continue;

				if (id == uid) {
					mode_to_access_flags(pa_entry->e_perm,
							     0777,
							     &access_bits);
/* …（同文件无关代码省略）… */
		if (others_ace) {
			ace = others_ace;
		} else {
			ksmbd_debug(SMB, "Can't find corresponding sid\n");
			rc = -EACCES;
			goto err_out;
		}
	}
/* …（同文件无关代码省略）… */
	}

	if (granted & ~access_bits) {
		ksmbd_debug(SMB, "Access denied with winACL, granted : %x, access_req : %x\n",
			    granted, le32_to_cpu(ace->access_req));
		rc = -EACCES;
		goto err_out;
	}
