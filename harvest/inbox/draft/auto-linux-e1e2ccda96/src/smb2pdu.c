// AUTO-DRAFT from torvalds/linux PR #7f75cd8ccd9588e969158c5dc92a8cc217663f5b
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
		if (!eabuf->EaNameLength)
			goto next;

		ksmbd_debug(SMB,
			    "name : <%s>, name_len : %u, value_len : %u, next : %u\n",
			    eabuf->name, eabuf->EaNameLength,
/* …（同文件无关代码省略）… */
	if (fp->cdoption == FILE_OPEN_LE) {
		if (!strcmp(stream_name, "AFP_AfpInfo") &&
		    test_share_config_flag(fp->tcon->share_conf,
					   KSMBD_SHARE_FLAG_TIME_MACHINE)) {
			/*
			 * Synthesize an empty AFP_AfpInfo xattr on first access.
			 * type=0/creator=0 tells macOS to use the file extension
/* …（同文件无关代码省略）… */
		return -EBADF;
	}

	rc = ksmbd_vfs_setxattr(idmap, path, xattr_stream_name, NULL, 0, 0, false);
	if (rc < 0)
		pr_err("Failed to store XATTR stream name :%d\n", rc);
/* …（同文件无关代码省略）… */
			file_info = FILE_OPENED;
			rc = ksmbd_vfs_getattr(&fp->filp->f_path, &stat);
			if (rc)
				goto err_out2;

			goto reconnected_fp;
		}
/* …（同文件无关代码省略）… */

			rc = ksmbd_vfs_getattr(&fp->filp->f_path, &stat);
			if (rc)
				goto err_out2;

			goto reconnected_fp;
		}
/* …（同文件无关代码省略）… */
				goto err_out2;
			}
		}
	}

	if (le32_to_cpu(req->CreateDisposition) >
/* …（同文件无关代码省略）… */
		maximal_access = daccess;
	}

	open_flags = smb2_create_open_flags(file_present, daccess,
					    req->CreateDisposition,
					    &may_flags,
/* …（同文件无关代码省略）… */
	if (rc) {
		if (rc == -EINVAL)
			rsp->hdr.Status = STATUS_INVALID_PARAMETER;
		else if (rc == -EOPNOTSUPP)
			rsp->hdr.Status = STATUS_NOT_SUPPORTED;
		else if ((rc == -EACCES || rc == -ESTALE || rc == -EXDEV) &&
/* …（同文件无关代码省略）… */
	}
	else if (rc == -EINVAL)
		rsp->hdr.Status = STATUS_INVALID_PARAMETER;
	else if (rc == -EMSGSIZE)
		rsp->hdr.Status = STATUS_INFO_LENGTH_MISMATCH;
	else if (rc == -ENOSPC || rc == -EFBIG)
