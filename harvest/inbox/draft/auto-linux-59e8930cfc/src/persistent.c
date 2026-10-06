// AUTO-DRAFT from torvalds/linux PR #2c3418fffa9d037b2038a6db48be63f9e2291806
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
static int key_create_persistent_register(struct user_namespace *ns)
{
	struct key *reg = keyring_alloc(".persistent_register",
					KUIDT_INIT(0), KGIDT_INIT(0),
					current_cred(),
					((KEY_POS_ALL & ~KEY_POS_SETATTR) |
					 KEY_USR_VIEW | KEY_USR_READ),
					KEY_ALLOC_NOT_IN_QUOTA, NULL, NULL);
	if (IS_ERR(reg))
		return PTR_ERR(reg);

	ns->persistent_keyring_register = reg;
	return 0;
}

/*
 * Create the persistent keyring for the specified user.
 *
 * Called with the namespace's sem locked for writing.
 */
static key_ref_t key_create_persistent(struct user_namespace *ns, kuid_t uid,
				       struct keyring_index_key *index_key)
{
	struct key *persistent;
	key_ref_t reg_ref, persistent_ref;

	if (!ns->persistent_keyring_register) {
		long err = key_create_persistent_register(ns);
		if (err < 0)
			return ERR_PTR(err);
	} else {
		reg_ref = make_key_ref(ns->persistent_keyring_register, true);
		persistent_ref = find_key_to_update(reg_ref, index_key);
		if (persistent_ref)
			return persistent_ref;
	}

	persistent = keyring_alloc(index_key->description,
/* …（同文件无关代码省略）… */
				   KEY_ALLOC_NOT_IN_QUOTA, NULL,
				   ns->persistent_keyring_register);
	if (IS_ERR(persistent))
		return ERR_CAST(persistent);

	return make_key_ref(persistent, true);
}

/*
/* …（同文件无关代码省略）… */
	key_ref_t reg_ref, persistent_ref;
	char buf[32];
	long ret;

	/* Look in the register if it exists */
	memset(&index_key, 0, sizeof(index_key));
/* …（同文件无关代码省略）… */
	 * also need to create the register.
	 */
	down_write(&ns->keyring_sem);
	persistent_ref = key_create_persistent(ns, uid, &index_key);
	up_write(&ns->keyring_sem);
	if (!IS_ERR(persistent_ref))
		goto found;

	return PTR_ERR(persistent_ref);

found:
	ret = key_task_permission(persistent_ref, current_cred(), KEY_NEED_LINK);
	if (ret == 0) {
		persistent = key_ref_to_ptr(persistent_ref);
		ret = key_link(key_ref_to_ptr(dest_ref), persistent);
		if (ret == 0) {
			key_set_timeout(persistent, persistent_keyring_expiry);
			ret = persistent->serial;
		}
	}

	key_ref_put(persistent_ref);
	return ret;
