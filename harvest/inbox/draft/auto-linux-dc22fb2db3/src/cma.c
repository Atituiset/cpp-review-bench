// AUTO-DRAFT from torvalds/linux PR #fcb92ecd733e11f37c32e7485da976a40d878e72
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
static int cma_set_default_qkey(struct rdma_id_private *id_priv)
{
	struct ib_sa_mcmember_rec rec;
	int ret = 0;

	switch (id_priv->id.ps) {
	case RDMA_PS_UDP:
	case RDMA_PS_IB:
		id_priv->qkey = RDMA_UDP_QKEY;
		break;
	case RDMA_PS_IPOIB:
		ib_addr_get_mgid(&id_priv->id.route.addr.dev_addr, &rec.mgid);
		ret = ib_sa_get_mcmember_rec(id_priv->id.device,
					     id_priv->id.port_num, &rec.mgid,
					     &rec);
		if (!ret)
			id_priv->qkey = be32_to_cpu(rec.qkey);
		break;
	default:
		break;
	}
	return ret;
}
/* …（同文件无关代码省略）… */
static int cma_set_qkey(struct rdma_id_private *id_priv, u32 qkey)
{
	if (!qkey ||
	    (id_priv->qkey && (id_priv->qkey != qkey)))
		return -EINVAL;

	id_priv->qkey = qkey;
	return 0;
}
/* …（同文件无关代码省略）… */
static int cma_send_sidr_rep(struct rdma_id_private *id_priv,
			     enum ib_cm_sidr_status status, u32 qkey,
			     const void *private_data, int private_data_len)
{
	struct ib_cm_sidr_rep_param rep;
	int ret;

	memset(&rep, 0, sizeof rep);
	rep.status = status;
	if (status == IB_SIDR_SUCCESS) {
		if (qkey)
			ret = cma_set_qkey(id_priv, qkey);
		else
			ret = cma_set_default_qkey(id_priv);
		if (ret)
			return ret;
		rep.qp_num = id_priv->qp_num;
		rep.qkey = id_priv->qkey;

		rep.ece.vendor_id = id_priv->ece.vendor_id;
		rep.ece.attr_mod = id_priv->ece.attr_mod;
	}

	rep.private_data = private_data;
	rep.private_data_len = private_data_len;

	trace_cm_send_sidr_rep(id_priv);
	return ib_send_cm_sidr_rep(id_priv->cm_id.ib, &rep);
}
/* …（同文件无关代码省略）… */
int rdma_reject(struct rdma_cm_id *id, const void *private_data,
		u8 private_data_len, u8 reason)
{
	struct rdma_id_private *id_priv;
	int ret;

	id_priv = container_of(id, struct rdma_id_private, id);
	if (!id_priv->cm_id.ib)
		return -EINVAL;

	if (rdma_cap_ib_cm(id->device, id->port_num)) {
		if (id->qp_type == IB_QPT_UD) {
			ret = cma_send_sidr_rep(id_priv, IB_SIDR_REJECT, 0,
						private_data, private_data_len);
		} else {
			trace_cm_send_rej(id_priv);
			ret = ib_send_cm_rej(id_priv->cm_id.ib, reason, NULL, 0,
					     private_data, private_data_len);
		}
	} else if (rdma_cap_iw_cm(id->device, id->port_num)) {
		ret = iw_cm_reject(id_priv->cm_id.iw,
				   private_data, private_data_len);
	} else {
		ret = -ENOSYS;
	}

	return ret;
}
EXPORT_SYMBOL(rdma_reject);

int rdma_disconnect(struct rdma_cm_id *id)
{
	struct rdma_id_private *id_priv;
/* …（同文件无关代码省略）… */
			goto out;
		/* Initiate or respond to a disconnect. */
		trace_cm_disconnect(id_priv);
		if (ib_send_cm_dreq(id_priv->cm_id.ib, NULL, 0)) {
			if (!ib_send_cm_drep(id_priv->cm_id.ib, NULL, 0))
				trace_cm_sent_drep(id_priv);
		} else {
			trace_cm_sent_dreq(id_priv);
		}
	} else if (rdma_cap_iw_cm(id->device, id->port_num)) {
		ret = iw_cm_disconnect(id_priv->cm_id.iw, 0);
	} else
