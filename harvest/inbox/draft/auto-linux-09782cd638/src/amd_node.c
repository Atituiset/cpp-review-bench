// AUTO-DRAFT from torvalds/linux PR #b4e7fc36e31f59b318d38afe621ac20650314d27
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static struct pci_dev *get_next_root(struct pci_dev *root)
{
	while ((root = pci_get_class(PCI_CLASS_BRIDGE_HOST << 8, root))) {
		/* Root device is Device 0 Function 0. */
		if (root->devfn)
			continue;

		if (root->vendor != PCI_VENDOR_ID_AMD &&
		    root->vendor != PCI_VENDOR_ID_HYGON)
			continue;

		break;
	}

	return root;
}
/* …（同文件无关代码省略）… */
static int __init amd_smn_init(void)
{
	u16 count, num_roots, roots_per_node, node, num_nodes;
	struct pci_dev *root;

	if (!cpu_feature_enabled(X86_FEATURE_ZEN))
		return 0;
/* …（同文件无关代码省略）… */
		return 0;

	num_roots = 0;
	root = NULL;
	while ((root = get_next_root(root))) {
		pci_dbg(root, "Reserving PCI config space\n");

/* …（同文件无关代码省略）… */

	count = 0;
	node = 0;
	root = NULL;
	while (node < num_nodes && (root = get_next_root(root))) {
		/* Use one root for each node and skip the rest. */
		if (count++ % roots_per_node)
			continue;

		pci_dbg(root, "is root for AMD node %u\n", node);
		amd_roots[node++] = root;
	}

	if (enable_dfs) {
