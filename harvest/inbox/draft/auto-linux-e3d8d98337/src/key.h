// AUTO-DRAFT from torvalds/linux PR #f1bfe4003916a6d56e43b08698f38bad4443bcf4
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdint.h>
  // <<< BUG ANCHOR
typedef uint32_t key_perm_t;
/* …（同文件无关代码省略）… */

#undef KEY_DEBUGGING

#define KEY_POS_VIEW	0x01000000	/* possessor can view a key's attributes */
#define KEY_POS_READ	0x02000000	/* possessor can read key payload / view keyring */
#define KEY_POS_WRITE	0x04000000	/* possessor can update key payload / add link to keyring */
/* …（同文件无关代码省略）… */
	kuid_t			uid;
	kgid_t			gid;
	key_perm_t		perm;		/* access permissions */
	unsigned short		quotalen;	/* length added to quota */
	unsigned short		datalen;	/* payload data length
						 * - may not match RCU dereferenced payload
						 * - payload should contain own length
						 */
