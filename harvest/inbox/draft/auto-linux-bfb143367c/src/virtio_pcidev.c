// AUTO-DRAFT from torvalds/linux PR #7cd80b8168c9269e4b11788ce815c0f446ab5b34
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
return 0;
}

static void __virtio_pcidev_virtio_platform_remove(struct virtio_device *vdev,
						   struct virtio_pcidev_device *dev)
{
	um_pci_platform_device_unregister(&dev->pdev);

	virtio_reset_device(vdev);
	vdev->config->del_vqs(vdev);

	kfree(dev);
}
	um_pci_platform_device_unregister(&dev->pdev);
err_reset:
	virtio_reset_device(vdev);
	vdev->config->del_vqs(vdev);
err_free:
	kfree(dev);
	return err;

err_reset:
	virtio_reset_device(vdev);
	vdev->config->del_vqs(vdev);
err_free:
	kfree(dev);
	return err;

	/* Stop all virtqueues */
	virtio_reset_device(vdev);
	dev->cmd_vq = NULL;
	dev->irq_vq = NULL;
	vdev->config->del_vqs(vdev);

	kfree(dev);
}
