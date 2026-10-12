// AUTO-DRAFT from torvalds/linux PR #5906f156e8818d29839d7d7280b691f627b59883
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
 * before returning to the caller, and @consumers will not be
 * changed.
 *
 * Return: Number of regulators on success, or a negative error number
 *	   on failure.
 */
int of_regulator_bulk_get_all(struct device *dev, struct device_node *np,
			      struct regulator_bulk_data **consumers)
{
	int num_consumers = 0;
	struct regulator *tmp;
	struct regulator_bulk_data *_consumers = NULL;
	struct property *prop;
	int i, n = 0, ret;
	char name[64];

	/*
	 * first pass: get numbers of xxx-supply
	 * second pass: fill consumers
	 */
restart:
/* …（同文件无关代码省略）… */
			continue;
		if (!_consumers) {
			num_consumers++;
			continue;
		} else {
			memcpy(name, prop->name, i);
			name[i] = '\0';
			tmp = regulator_get(dev, name);
			if (IS_ERR(tmp)) {
				ret = PTR_ERR(tmp);
				goto error;
			}
			_consumers[n].consumer = tmp;
			n++;
			continue;
		}
/* …（同文件无关代码省略）… */
	}
	if (num_consumers == 0)
		return 0;
	_consumers = kmalloc_objs(struct regulator_bulk_data, num_consumers);
	if (!_consumers)
		return -ENOMEM;
	goto restart;

error:
