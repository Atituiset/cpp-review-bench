// AUTO-DRAFT from torvalds/linux PR #af32da41b0327b9c6a37856ba82b6760d6c8d10e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
void iavf_schedule_aq_request(struct iavf_adapter *adapter, u64 flags)
{
	adapter->aq_required |= flags;
	mod_delayed_work(adapter->wq, &adapter->watchdog_task, 0);
}
/* …（同文件无关代码省略）… */
static struct
iavf_mac_filter *iavf_find_filter(struct iavf_adapter *adapter,
				  const u8 *macaddr)
{
	struct iavf_mac_filter *f;

	if (!macaddr)
		return NULL;

	list_for_each_entry(f, &adapter->mac_filter_list, list) {
		if (ether_addr_equal(macaddr, f->macaddr))
			return f;
	}
	return NULL;
}
/* …（同文件无关代码省略）… */
struct iavf_mac_filter *iavf_add_filter(struct iavf_adapter *adapter,
					const u8 *macaddr)
{
	struct iavf_mac_filter *f;

	if (!macaddr)
		return NULL;

	f = iavf_find_filter(adapter, macaddr);
	if (!f) {
		f = kzalloc_obj(*f, GFP_ATOMIC);
		if (!f)
			return f;

		ether_addr_copy(f->macaddr, macaddr);

		list_add_tail(&f->list, &adapter->mac_filter_list);
		f->add = true;
		f->add_handled = false;
		f->is_new_mac = true;
		f->is_primary = ether_addr_equal(macaddr, adapter->hw.mac.addr);
		adapter->aq_required |= IAVF_FLAG_AQ_ADD_MAC_FILTER;
	} else {
		f->remove = false;
	}

	return f;
}
/* …（同文件无关代码省略）… */
static int iavf_replace_primary_mac(struct iavf_adapter *adapter,
				    const u8 *new_mac)
{
	struct iavf_hw *hw = &adapter->hw;
	struct iavf_mac_filter *new_f;
	struct iavf_mac_filter *old_f;

	spin_lock_bh(&adapter->mac_vlan_list_lock);

	new_f = iavf_add_filter(adapter, new_mac);
	if (!new_f) {
		spin_unlock_bh(&adapter->mac_vlan_list_lock);
		return -ENOMEM;
	}

	old_f = iavf_find_filter(adapter, hw->mac.addr);
	if (old_f) {
		old_f->is_primary = false;
		old_f->remove = true;
		adapter->aq_required |= IAVF_FLAG_AQ_DEL_MAC_FILTER;
	}
	/* Always send the request to add if changing primary MAC,
	 * even if filter is already present on the list
	 */
	new_f->is_primary = true;
	new_f->add = true;
	ether_addr_copy(hw->mac.addr, new_mac);

	spin_unlock_bh(&adapter->mac_vlan_list_lock);

	/* schedule the watchdog task to immediately process the request */
	iavf_schedule_aq_request(adapter, IAVF_FLAG_AQ_ADD_MAC_FILTER);
	return 0;
}
/* …（同文件无关代码省略）… */
static bool iavf_is_mac_set_handled(struct net_device *netdev,
				    const u8 *macaddr)
{
	struct iavf_adapter *adapter = netdev_priv(netdev);
	struct iavf_mac_filter *f;
	bool ret = false;

	spin_lock_bh(&adapter->mac_vlan_list_lock);

	f = iavf_find_filter(adapter, macaddr);

	if (!f || (!f->add && f->add_handled))
		ret = true;

	spin_unlock_bh(&adapter->mac_vlan_list_lock);

	return ret;
}

/**
 * iavf_set_mac - NDO callback to set port MAC address
 * @netdev: network interface device structure
/* …（同文件无关代码省略）… */
		return -EADDRNOTAVAIL;

	ret = iavf_replace_primary_mac(adapter, addr->sa_data);

	if (ret)
		return ret;

	ret = wait_event_interruptible_timeout(adapter->vc_waitqueue,
					       iavf_is_mac_set_handled(netdev, addr->sa_data),
					       msecs_to_jiffies(2500));

	/* If ret < 0 then it means wait was interrupted.
	 * If ret == 0 then it means we got a timeout.
	 * else it means we got response for set MAC from PF,
	 * check if netdev MAC was updated to requested MAC,
	 * if yes then set MAC succeeded otherwise it failed return -EACCES
	 */
	if (ret < 0)
		return ret;

	if (!ret)
		return -EAGAIN;

	if (!ether_addr_equal(netdev->dev_addr, addr->sa_data))
		return -EACCES;
/* …（同文件无关代码省略）… */
	/* Setup the wait queue for indicating transition to down status */
	init_waitqueue_head(&adapter->down_waitqueue);

	/* Setup the wait queue for indicating virtchannel events */
	init_waitqueue_head(&adapter->vc_waitqueue);

	INIT_LIST_HEAD(&adapter->ptp.aq_cmds);
	init_waitqueue_head(&adapter->ptp.phc_time_waitqueue);
	mutex_init(&adapter->ptp.aq_cmd_lock);
