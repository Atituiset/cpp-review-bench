// AUTO-DRAFT from torvalds/linux PR #d89e1d186ec9271bf78e62771779e9fc03bf0fc3
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
		if (node->hkdf_context == hkdf_context &&
		    node->mode_num == mode_num &&
		    node->data_unit_bits == ci->ci_data_unit_bits &&
		    fscrypt_is_key_prepared(&node->key, ci))
			return &node->key;
	}
	return NULL;
}
