// AUTO-DRAFT from torvalds/linux PR #f909e534f5e34feb2ea2701d54df63e21aa506fc
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
			spin_unlock(&conn->session->frwd_lock);
			spin_lock(&conn->session->back_lock);
		} else {
			struct iscsi_task *task;
			/*
			 * Our nop as ping got dropped. We know the target
			 * and transport are ok so just clean up
			 */
			task = iscsi_itt_to_task(conn, rejected_pdu.itt);
			if (!task) {
				iscsi_conn_printk(KERN_ERR, conn,
						 "Invalid pdu reject. Could "
						 "not lookup rejected task.\n");
/* …（同文件无关代码省略）… */
struct iscsi_task *iscsi_itt_to_task(struct iscsi_conn *conn, itt_t itt)
{
	struct iscsi_session *session = conn->session;
	int i;

	if (itt == RESERVED_ITT)
		return NULL;

	if (session->tt->parse_pdu_itt)
		session->tt->parse_pdu_itt(conn, itt, &i, NULL);
	else
		i = get_itt(itt);
	if (i >= session->cmds_max)
		return NULL;

	return session->cmds[i];
}
/* …（同文件无关代码省略）… */
	case ISCSI_OP_SCSI_TMFUNC_RSP:
	case ISCSI_OP_NOOP_IN:
		task = iscsi_itt_to_task(conn, hdr->itt);
		if (!task)
			return ISCSI_ERR_BAD_ITT;
		break;
	default:
/* …（同文件无关代码省略）… */
		return ISCSI_ERR_BAD_ITT;
	}

	if (i >= session->cmds_max) {
		iscsi_conn_printk(KERN_ERR, conn,
				  "received invalid itt index %u (max cmds "
				   "%u.\n", i, session->cmds_max);
