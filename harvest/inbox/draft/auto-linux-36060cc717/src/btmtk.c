// AUTO-DRAFT from torvalds/linux PR #f66469d9905b39d2c260a15d7d6db450f4db67c1
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#include <linux/usb.h>
#include <linux/iopoll.h>
#include <linux/overflow.h>
#include <linux/unaligned.h>

#include <net/bluetooth/bluetooth.h>
/* …（同文件无关代码省略）… */
#define BTMTK_CBMCU_FLAG_ENABLE_PATCH	0xF1

/* It is for mt79xx iso data transmission setting */
#define MTK_ISO_THRESHOLD	264

struct btmtk_patch_header {
	u8 datetime[16];
/* …（同文件无关代码省略）… */
static int __set_mtk_intr_interface(struct hci_dev *hdev)
{
	struct btmtk_data *btmtk_data = hci_get_priv(hdev);
	struct usb_interface *intf = btmtk_data->isopkt_intf;
	int err;

	if (!btmtk_data->isopkt_intf)
		return -ENODEV;

	err = usb_set_interface(btmtk_data->udev, MTK_ISO_IFNUM,
			       (intf->num_altsetting > 1) ? 1 : 0);
	if (err < 0) {
		bt_dev_err(hdev, "setting interface failed (%d)", -err);
		return err;
	}

	err = usb_find_common_endpoints(intf->cur_altsetting, NULL, NULL,
					&btmtk_data->isopkt_rx_ep,
					&btmtk_data->isopkt_tx_ep);
	if (err) {
		bt_dev_err(hdev, "invalid interrupt descriptors");
		return -ENODEV;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static int btmtk_recv_isopkt(struct hci_dev *hdev, void *buffer, int count)
{
	struct btmtk_data *btmtk_data = hci_get_priv(hdev);
	struct sk_buff *skb;
	unsigned long flags;
	int err = 0;

	spin_lock_irqsave(&btmtk_data->isorxlock, flags);
	skb = btmtk_data->isopkt_skb;

	while (count) {
		int len;

		if (!skb) {
			skb = bt_skb_alloc(HCI_MAX_ISO_SIZE, GFP_ATOMIC);
			if (!skb) {
				err = -ENOMEM;
				break;
			}

			hci_skb_pkt_type(skb) = HCI_ISODATA_PKT;
			hci_skb_expect(skb) = HCI_ISO_HDR_SIZE;
		}

		len = min_t(uint, hci_skb_expect(skb), count);
		skb_put_data(skb, buffer, len);

		count -= len;
		buffer += len;
		hci_skb_expect(skb) -= len;

		if (skb->len == HCI_ISO_HDR_SIZE) {
			__le16 dlen = ((struct hci_iso_hdr *)skb->data)->dlen;

			/* Complete ISO header */
			hci_skb_expect(skb) = __le16_to_cpu(dlen);

			if (skb_tailroom(skb) < hci_skb_expect(skb)) {
				kfree_skb(skb);
				skb = NULL;

				err = -EILSEQ;
				break;
			}
		}

		if (!hci_skb_expect(skb)) {
			/* Complete frame */
			hci_recv_frame(hdev, skb);
			skb = NULL;
		}
	}

	btmtk_data->isopkt_skb = skb;
	spin_unlock_irqrestore(&btmtk_data->isorxlock, flags);

	return err;
}
/* …（同文件无关代码省略）… */
static void btmtk_intr_complete(struct urb *urb)
{
	struct hci_dev *hdev = urb->context;
	struct btmtk_data *btmtk_data = hci_get_priv(hdev);
	int err;

	BT_DBG("%s urb %p status %d count %d", hdev->name, urb, urb->status,
	       urb->actual_length);

	if (!test_bit(HCI_RUNNING, &hdev->flags))
		return;

	if (hdev->suspended)
		return;

	if (urb->status == 0) {
		hdev->stat.byte_rx += urb->actual_length;

		if (btmtk_recv_isopkt(hdev, urb->transfer_buffer,
				      urb->actual_length) < 0) {
			bt_dev_err(hdev, "corrupted iso packet");
			hdev->stat.err_rx++;
		}
	} else if (urb->status == -ENOENT) {
		/* Avoid suspend failed when usb_kill_urb */
		return;
	}

	usb_mark_last_busy(btmtk_data->udev);
	usb_anchor_urb(urb, &btmtk_data->isopkt_anchor);

	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (err < 0) {
		/* -EPERM: urb is being killed;
		 * -ENODEV: device got disconnected
		 */
		if (err != -EPERM && err != -ENODEV)
			bt_dev_err(hdev, "urb %p failed to resubmit (%d)",
				   urb, -err);
		if (err != -EPERM)
			hci_cmd_sync_cancel(hdev, -err);
		usb_unanchor_urb(urb);
	}
}
/* …（同文件无关代码省略）… */
static int btmtk_submit_intr_urb(struct hci_dev *hdev, gfp_t mem_flags)
{
	struct btmtk_data *btmtk_data = hci_get_priv(hdev);
	unsigned char *buf;
	unsigned int pipe;
	struct urb *urb;
	int err, size;

	BT_DBG("%s", hdev->name);

	if (!btmtk_data->isopkt_rx_ep)
		return -ENODEV;

	urb = usb_alloc_urb(0, mem_flags);
	if (!urb)
		return -ENOMEM;
	size = le16_to_cpu(btmtk_data->isopkt_rx_ep->wMaxPacketSize);

	buf = kmalloc(size, mem_flags);
	if (!buf) {
		usb_free_urb(urb);
		return -ENOMEM;
	}

	pipe = usb_rcvintpipe(btmtk_data->udev,
			      btmtk_data->isopkt_rx_ep->bEndpointAddress);

	usb_fill_int_urb(urb, btmtk_data->udev, pipe, buf, size,
			 btmtk_intr_complete, hdev,
			 btmtk_data->isopkt_rx_ep->bInterval);

	urb->transfer_flags |= URB_FREE_BUFFER;

	usb_mark_last_busy(btmtk_data->udev);
	usb_anchor_urb(urb, &btmtk_data->isopkt_anchor);

	err = usb_submit_urb(urb, mem_flags);
	if (err < 0) {
		if (err != -EPERM && err != -ENODEV)
			bt_dev_err(hdev, "urb %p submission failed (%d)",
				   urb, -err);
		usb_unanchor_urb(urb);
	}

	usb_free_urb(urb);

	return err;
}
/* …（同文件无关代码省略）… */
static int btmtk_usb_isointf_init(struct hci_dev *hdev)
{
	struct btmtk_data *btmtk_data = hci_get_priv(hdev);
	u8 iso_param[2] = { 0x08, 0x01 };
	struct sk_buff *skb;
	int err;

	spin_lock_init(&btmtk_data->isorxlock);

	__set_mtk_intr_interface(hdev);

	err = btmtk_submit_intr_urb(hdev, GFP_KERNEL);
	if (err < 0) {
/* …（同文件无关代码省略）… */
		return err;
	}

	skb = __hci_cmd_sync(hdev, 0xfd98, sizeof(iso_param), iso_param,
			     HCI_INIT_TIMEOUT);
	if (IS_ERR(skb)) {
		bt_dev_err(hdev, "Failed to apply iso setting (%ld)", PTR_ERR(skb));
		return PTR_ERR(skb);
	}
