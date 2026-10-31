// AUTO-DRAFT from torvalds/linux PR #60aba79a50127e92d8c4a663d35b855e52a9da01
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
  // <<< BUG ANCHOR
static ssize_t query_data(char *buf, size_t buf_len,
			  char *query, size_t query_len)
{
	char *out;
	const char *key;
	struct label_it i;
	struct aa_label *label, *curr;
/* …（同文件无关代码省略）… */
	if (buf_len < sizeof(bytes) + sizeof(blocks))
		return -EINVAL; /* not enough space */

	curr = begin_current_label_crit_section(&needput);
	label = aa_label_parse(curr, query, GFP_KERNEL, false, false);
	end_current_label_crit_section(curr, needput);
	if (IS_ERR(label))
		return PTR_ERR(label);

	/* We are going to leave space for two numbers. The first is the total
	 * number of bytes we are writing after the first number. This is so
/* …（同文件无关代码省略）… */
		if (!profile->data)
			continue;

		data = rhashtable_lookup_fast(profile->data, &key,
					      profile->data->p);

		if (data) {
			if (out + sizeof(outle32) + data->size > buf +
			    buf_len) {
				aa_put_label(label);
				return -EINVAL; /* not enough space */
			}
			outle32 = __cpu_to_le32(data->size);
/* …（同文件无关代码省略）… */
		}
	}
	aa_put_label(label);

	outle32 = __cpu_to_le32(out - buf - sizeof(bytes));
	memcpy(buf, &outle32, sizeof(outle32));
