// AUTO-DRAFT from torvalds/linux PR #fcb92ecd733e11f37c32e7485da976a40d878e72
  // <<< BUG ANCHOR
	if (nla_put_u32(msg, RDMA_NLDEV_ATTR_RES_CQN, res->id))
		return -EMSGSIZE;
	if (!rdma_is_kernel_res(res) &&
	    nla_put_u32(msg, RDMA_NLDEV_ATTR_RES_CTXN,
			cq->uobject->uevent.uobject.context->res.id))
		return -EMSGSIZE;
/* …（同文件无关代码省略）… */
	if (nla_put_u32(msg, RDMA_NLDEV_ATTR_RES_PDN, res->id))
		goto err;

	if (!rdma_is_kernel_res(res) &&
	    nla_put_u32(msg, RDMA_NLDEV_ATTR_RES_CTXN,
			pd->uobject->context->res.id))
		goto err;
/* …（同文件无关代码省略）… */
		mutex_lock(&nldev_dellink_mutex);
		err = device->link_ops->dellink(device);
		mutex_unlock(&nldev_dellink_mutex);
		if (err)
			return err;
	}

	ib_unregister_device_and_put(device);
