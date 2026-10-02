// AUTO-DRAFT from torvalds/linux PR #ce1e0223d8ad4211275c82a17ed6d43ab81e13d9
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
struct target {
	struct device_node *np;
	bool in_livetree;
};
/* …（同文件无关代码省略）… */
	if (!target_path)
		return NULL;
	target_path_len = strlen(target_path);

	new_prop = kzalloc_obj(*new_prop);
	if (!new_prop)
/* …（同文件无关代码省略）… */
		return -ENOMEM;

	if (!prop) {
		if (!target->in_livetree) {
			new_prop->next = target->np->deadprops;
			target->np->deadprops = new_prop;
		}
		ret = of_changeset_add_property(&ovcs->cset, target->np,
						new_prop);
	} else {
		ret = of_changeset_update_property(&ovcs->cset, target->np,
						   new_prop);
/* …（同文件无关代码省略）… */
err_out:
	pr_err("%s() failed, ret = %d\n", __func__, ret);

	return ret;
}

/* …（同文件无关代码省略）… */
	if (ovcs->cset.entries.next)
		of_changeset_destroy(&ovcs->cset);

	if (ovcs->id) {
		idr_remove(&ovcs_idr, ovcs->id);
		list_del(&ovcs->ovcs_list);
		ovcs->id = 0;
