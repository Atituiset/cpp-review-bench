// AUTO-DRAFT from torvalds/linux PR #9a58da80053f992b285b6b7bebc694b0f284c443
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <string.h>
  // <<< BUG ANCHOR

static int parse_veto_list(struct ksmbd_share_config *share,
			   char *veto_list,
			   int veto_list_sz)
{
	int sz = 0;

	if (!veto_list_sz)
		return 0;

	while (veto_list_sz > 0) {
		struct ksmbd_veto_pattern *p;

		sz = strlen(veto_list);
		if (!sz)
			break;

		p = kzalloc_obj(struct ksmbd_veto_pattern, KSMBD_DEFAULT_GFP);
		if (!p)
			return -ENOMEM;

		p->pattern = kstrdup(veto_list, KSMBD_DEFAULT_GFP);
		if (!p->pattern) {
			kfree(p);
			return -ENOMEM;
		}

		list_add(&p->list, &share->veto_list);

		veto_list += sz + 1;
		veto_list_sz -= (sz + 1);
	}
/* …（同文件无关代码省略）… */
	}

	if (!test_share_config_flag(share, KSMBD_SHARE_FLAG_PIPE)) {
		int path_len = PATH_MAX;

		if (resp->payload_sz)
			path_len = resp->payload_sz - resp->veto_list_sz;

		share->path = kstrndup(ksmbd_share_config_path(resp), path_len,
				      KSMBD_DEFAULT_GFP);
		if (!share->path) {
			ret = -ENOMEM;
		} else {
			ret = 0;
			share->path_sz = strlen(share->path);
			while (share->path_sz > 1 &&
			       share->path[share->path_sz - 1] == '/')
