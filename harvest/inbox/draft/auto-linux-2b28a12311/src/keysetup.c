// AUTO-DRAFT from torvalds/linux PR #fb52ae0e9afb44d5f06bcf855b490584dad66687
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	if (res)
		goto out;

	/*
	 * For existing inodes, multiple tasks may race to set the inode's
	 * fscrypt info pointer.  So use cmpxchg_release().  This pairs with the
/* …（同文件无关代码省略）… */
		 * crypt_info.  Now link it into the master key's inode list.
		 */
		if (mk) {
			crypt_info->ci_master_key = mk;
			refcount_inc(&mk->mk_active_refs);
			spin_lock(&mk->mk_decrypted_inodes_lock);
			list_add(&crypt_info->ci_master_key_link,
				 &mk->mk_decrypted_inodes);
			spin_unlock(&mk->mk_decrypted_inodes_lock);
		}
		crypt_info = NULL;
	}
	res = 0;
out:
