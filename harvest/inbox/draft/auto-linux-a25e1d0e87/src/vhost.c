// AUTO-DRAFT from torvalds/linux PR #9f2516f45b7cfca237f4d8e1c91c1f307e295dc5
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static void vhost_detach_mm(struct vhost_dev *dev)
{
	if (!dev->mm)
		return;

	if (dev->use_worker)
		mmput(dev->mm);
	else
		mmdrop(dev->mm);

	dev->mm = NULL;
}
/* …（同文件无关代码省略）… */
	kfree(worker);
}

static void vhost_workers_free(struct vhost_dev *dev)
{
	struct vhost_worker *worker;
	unsigned long i;
/* …（同文件无关代码省略）… */

	for (i = 0; i < dev->nvqs; i++)
		rcu_assign_pointer(dev->vqs[i]->worker, NULL);
	/*
	 * Free the default worker we created and cleanup workers userspace
	 * created but couldn't clean up (it forgot or crashed).
/* …（同文件无关代码省略）… */
static struct vhost_iotlb *iotlb_alloc(void)
{
	if (max_iotlb_entries <= 0)
		return NULL;

	return vhost_iotlb_alloc(max_iotlb_entries,
				 VHOST_IOTLB_FLAG_RETIRE);
}
/* …（同文件无关代码省略）… */
struct vhost_iotlb *vhost_dev_reset_owner_prepare(void)
{
	return iotlb_alloc();
}
EXPORT_SYMBOL_GPL(vhost_dev_reset_owner_prepare);

/* Caller should have device mutex */
void vhost_dev_reset_owner(struct vhost_dev *dev, struct vhost_iotlb *umem)
{
	int i;

	vhost_dev_cleanup(dev);

	dev->fork_owner = fork_from_owner_default;
	dev->umem = umem;
/* …（同文件无关代码省略）… */
static void vhost_free_msg_locked(struct vhost_msg_node *node)
{
	if (node->vq->iotlb_miss == node)
		node->vq->iotlb_miss = NULL;
	kfree(node);
}
/* …（同文件无关代码省略）… */
void vhost_clear_msg(struct vhost_dev *dev)
{
	struct vhost_msg_node *node, *n;

	spin_lock(&dev->iotlb_lock);

	list_for_each_entry_safe(node, n, &dev->read_list, node) {
		list_del(&node->node);
		vhost_free_msg_locked(node);
	}

	list_for_each_entry_safe(node, n, &dev->pending_list, node) {
		list_del(&node->node);
		vhost_free_msg_locked(node);
	}

	spin_unlock(&dev->iotlb_lock);
}
EXPORT_SYMBOL_GPL(vhost_clear_msg);

void vhost_dev_cleanup(struct vhost_dev *dev)
{
	int i;

/* …（同文件无关代码省略）… */
	dev->iotlb = NULL;
	vhost_clear_msg(dev);
	wake_up_interruptible_poll(&dev->wait, EPOLLIN | EPOLLRDNORM);
	vhost_workers_free(dev);
	vhost_detach_mm(dev);
}
EXPORT_SYMBOL_GPL(vhost_dev_cleanup);
