// AUTO-DRAFT from torvalds/linux PR #ce1e0223d8ad4211275c82a17ed6d43ab81e13d9
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
bool of_node_name_eq(const struct device_node *np, const char *name)
{
	const char *node_name;
	size_t len;

	if (!np)
		return false;

	node_name = kbasename(np->full_name);
	len = strchrnul(node_name, '@') - node_name;

	return (strlen(name) == len) && (strncmp(node_name, name, len) == 0);
}
/* …（同文件无关代码省略）… */
static bool __of_node_is_type(const struct device_node *np, const char *type)
{
	const char *match;
	int len;

	if (!np || !type)
		return false;

	match = __of_get_property(np, "device_type", &len);
	if (!match || len <= 0 || strnlen(match, len) >= len)
		return false;

	return !strcmp(match, type);
}

#define EXCLUDED_DEFAULT_CELLS_PLATFORMS ( \
	IS_ENABLED(CONFIG_SPARC) || \
	of_find_compatible_node(NULL, NULL, "coreboot") \
)

int of_bus_n_addr_cells(struct device_node *np)
{
	u32 cells;

	for (; np; np = np->parent) {
		if (!of_property_read_u32(np, "#address-cells", &cells))
			return cells;
		/*
		 * Default root value and walking parent nodes for "#address-cells"
		 * is deprecated. Any platforms which hit this warning should
		 * be added to the excluded list.
		 */
		WARN_ONCE(!EXCLUDED_DEFAULT_CELLS_PLATFORMS,
			  "Missing '#address-cells' in %pOF\n", np);
	}
	return OF_ROOT_NODE_ADDR_CELLS_DEFAULT;
}
/* …（同文件无关代码省略）… */
static struct property *__of_find_property(const struct device_node *np,
					   const char *name, int *lenp)
{
	struct property *pp;

	if (!np)
		return NULL;

	for (pp = np->properties; pp; pp = pp->next) {
		if (of_prop_cmp(pp->name, name) == 0) {
			if (lenp)
				*lenp = pp->length;
			break;
		}
	}

	return pp;
}
/* …（同文件无关代码省略）… */
const void *__of_get_property(const struct device_node *np,
			      const char *name, int *lenp)
{
	const struct property *pp = __of_find_property(np, name, lenp);

	return pp ? pp->value : NULL;
}
/* …（同文件无关代码省略）… */
static int __of_device_is_compatible(const struct device_node *device,
				     const char *compat, const char *type, const char *name)
{
	const struct property *prop;
	const char *cp;
	int index = 0, score = 0;

	/* Compatible match has highest priority */
	if (compat && compat[0]) {
		prop = __of_find_property(device, "compatible", NULL);
		for (cp = of_prop_next_string(prop, NULL); cp;
		     cp = of_prop_next_string(prop, cp), index++) {
			if (of_compat_cmp(cp, compat, strlen(compat)) == 0) {
				score = INT_MAX/2 - (index << 2);
				break;
			}
		}
		if (!score)
			return 0;
	}

	/* Matching type is better than matching name */
	if (type && type[0]) {
		if (!__of_node_is_type(device, type))
			return 0;
		score += 2;
	}

	/* Matching name is a bit better than not */
	if (name && name[0]) {
		if (!of_node_name_eq(device, name))
			return 0;
		score++;
	}

	return score;
}
/* …（同文件无关代码省略）… */
struct device_node *of_find_compatible_node(struct device_node *from,
	const char *type, const char *compatible)
{
	struct device_node *np;
	unsigned long flags;

	raw_spin_lock_irqsave(&devtree_lock, flags);
	for_each_of_allnodes_from(from, np)
		if (__of_device_is_compatible(np, compatible, type, NULL) &&
		    of_node_get(np))
			break;
	of_node_put(from);
	raw_spin_unlock_irqrestore(&devtree_lock, flags);
	return np;
}
/* …（同文件无关代码省略）… */
	for_each_of_allnodes_from(from, np) {
		if (__of_find_property(np, prop_name, NULL)) {
			of_node_get(np);
			break;
		}
	}
