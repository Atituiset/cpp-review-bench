// AUTO-DRAFT from torvalds/linux PR #4dd16e31135ea8437a720222df34e8745f385d6d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/xarray.h>
#include "ice_adapter.h"
#include "ice.h"
/* …（同文件无关代码省略）… */
#define ICE_ADAPTER_FIXED_INDEX	BIT_ULL(63)
/* …（同文件无关代码省略）… */
#define ICE_ADAPTER_INDEX_E825C	\
	(ICE_DEV_ID_E825C_BACKPLANE | ICE_ADAPTER_FIXED_INDEX)
/* …（同文件无关代码省略）… */
static u64 ice_adapter_index(struct pci_dev *pdev)
{
	switch (pdev->device) {
	case ICE_DEV_ID_E825C_BACKPLANE:
	case ICE_DEV_ID_E825C_QSFP:
	case ICE_DEV_ID_E825C_SFP:
	case ICE_DEV_ID_E825C_SGMII:
		/* E825C devices have multiple NACs which are connected to the
		 * same clock source, and which must share the same
		 * ice_adapter structure. We can't use the serial number since
		 * each NAC has its own NVM generated with its own unique
		 * Device Serial Number. Instead, rely on the embedded nature
		 * of the E825C devices, and use a fixed index. This relies on
		 * the fact that all E825C physical functions in a given
		 * system are part of the same overall device.
		 */
		return ICE_ADAPTER_INDEX_E825C;
	default:
		return pci_get_dsn(pdev) & ~ICE_ADAPTER_FIXED_INDEX;
	}
}
/* …（同文件无关代码省略）… */
static struct ice_adapter *ice_adapter_new(struct pci_dev *pdev)
{
	struct ice_adapter *adapter;

	adapter = kzalloc_obj(*adapter);
	if (!adapter)
		return NULL;

	adapter->index = ice_adapter_index(pdev);
	spin_lock_init(&adapter->ptp_gltsyn_time_lock);
	spin_lock_init(&adapter->txq_ctx_lock);
	for (int i = 0; i < ARRAY_SIZE(adapter->cpi_phy_lock); i++)
		mutex_init(&adapter->cpi_phy_lock[i]);
	refcount_set(&adapter->refcount, 1);

	mutex_init(&adapter->ports.lock);
	INIT_LIST_HEAD(&adapter->ports.ports);

	return adapter;
}

static void ice_adapter_free(struct ice_adapter *adapter)
{
	WARN_ON(!list_empty(&adapter->ports.ports));
	for (int i = 0; i < ARRAY_SIZE(adapter->cpi_phy_lock); i++)
		mutex_destroy(&adapter->cpi_phy_lock[i]);
	mutex_destroy(&adapter->ports.lock);

	kfree(adapter);
}
