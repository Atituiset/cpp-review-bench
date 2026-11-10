// AUTO-DRAFT from torvalds/linux PR #f1bfe4003916a6d56e43b08698f38bad4443bcf4
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
	}

	desclen = strlen(desc);
	quotalen = desclen + 1 + type->def_datalen;

	/* get hold of the key tracking for this user */
/* …（同文件无关代码省略）… */
static void mark_key_instantiated(struct key *key, int reject_error)
{
	/* Commit the payload before setting the state; barrier versus
	 * key_read_state().
	 */
	smp_store_release(&key->state,
			  (reject_error < 0) ? reject_error : KEY_IS_POSITIVE);
}
/* …（同文件无关代码省略）… */
	key_ref_t key_ref;
	int ret;
	struct key_restriction *restrict_link = NULL;

	/* look up the key type to see if it's one of the registered kernel
	 * types */
/* …（同文件无关代码省略）… */
		if (!index_key.description)
			goto error_free_prep;
	}
	index_key.desc_len = strlen(index_key.description);
	key_set_index_key(&index_key);

	ret = __key_link_lock(keyring, &index_key);
/* …（同文件无关代码省略）… */
int key_update(key_ref_t key_ref, const void *payload, size_t plen)
{
	struct key_preparsed_payload prep;
	struct key *key = key_ref_to_ptr(key_ref);
	int ret;

	key_check(key);

	/* the key must be writable */
	ret = key_permission(key_ref, KEY_NEED_WRITE);
	if (ret < 0)
		return ret;

	/* attempt to update it if supported */
	if (!key->type->update)
		return -EOPNOTSUPP;

	memset(&prep, 0, sizeof(prep));
	prep.data = payload;
	prep.datalen = plen;
	prep.quotalen = key->type->def_datalen;
	prep.expiry = TIME64_MAX;
	if (key->type->preparse) {
		ret = key->type->preparse(&prep);
		if (ret < 0)
			goto error;
	}

	down_write(&key->sem);

	ret = key->type->update(key, &prep);
	if (ret == 0) {
		/* Updating a negative key positively instantiates it */
		mark_key_instantiated(key, 0);
		notify_key(key, NOTIFY_KEY_UPDATED, 0);
	}

	up_write(&key->sem);

error:
	if (key->type->preparse)
		key->type->free_preparse(&prep);
	return ret;
}
EXPORT_SYMBOL(key_update);
