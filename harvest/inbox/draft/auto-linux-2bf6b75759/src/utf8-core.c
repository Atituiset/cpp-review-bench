// AUTO-DRAFT from torvalds/linux PR #1dbb7422053fa0f4fb5f9acc3931617fc0091939
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/parser.h>
#include <linux/errno.h>
#include <linux/stringhash.h>
/* …（同文件无关代码省略）… */
static const struct utf8data *find_table_version(const struct utf8data *table,
		size_t nr_entries, unsigned int version)
{
	size_t i = nr_entries - 1;

	while (version < table[i].maxage)
		i--;
	if (version > table[i].maxage)
		return NULL;
	return &table[i];
}
/* …（同文件无关代码省略）… */
struct unicode_map *utf8_load(unsigned int version)
{
	struct unicode_map *um;

	um = kzalloc_obj(struct unicode_map);
	if (!um)
		return ERR_PTR(-ENOMEM);
	um->version = version;

	um->tables = symbol_request(utf8_data_table);
	if (!um->tables)
		goto out_free_um;

	if (!utf8version_is_supported(um, version))
		goto out_symbol_put;
	um->ntab[UTF8_NFDI] = find_table_version(um->tables->utf8nfdidata,
			um->tables->utf8nfdidata_size, um->version);
	if (!um->ntab[UTF8_NFDI])
		goto out_symbol_put;
	um->ntab[UTF8_NFDICF] = find_table_version(um->tables->utf8nfdicfdata,
			um->tables->utf8nfdicfdata_size, um->version);
	if (!um->ntab[UTF8_NFDICF])
		goto out_symbol_put;
	return um;

out_symbol_put:
	symbol_put(utf8_data_table);
out_free_um:
	kfree(um);
	return ERR_PTR(-EINVAL);
}
EXPORT_SYMBOL(utf8_load);

void utf8_unload(struct unicode_map *um)
{
	if (um) {
		symbol_put(utf8_data_table);
		kfree(um);
	}
}
EXPORT_SYMBOL(utf8_unload);
