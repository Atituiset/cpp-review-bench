// AUTO-DRAFT from torvalds/linux PR #9a32e0754d637c1d386a533ae36e41c8f4be8b10
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#define DBG_DUMP	BIT(0) /* Data transmission dump. */
/* …（同文件无关代码省略）… */
#define DBG_DATA	BIT(2) /* Data transmission details. */
#define DBG_ERRORS	BIT(3) /* Details for fail conditions. */
#define DBG_TTY		BIT(4) /* Transmission statistics for DLCI TTYs. */
#define DBG_PAYLOAD	BIT(5) /* Limits DBG_DUMP to payload frames. */
/* …（同文件无关代码省略）… */
struct gsm_msg {
	struct list_head list;
	u8 addr;		/* DLCI address + flags */
	u8 ctrl;		/* Control byte + flags */
	unsigned int len;	/* Length of data block (can be zero) */
	u8 *data;	/* Points into buffer but not at the start */
	u8 buffer[];
};
/* …（同文件无关代码省略）… */
enum gsm_dlci_state {
	DLCI_CLOSED,
	DLCI_WAITING_CONFIG,	/* Waiting for DLCI configuration from user */
	DLCI_CONFIGURE,		/* Sending PN (for adaption > 1) */
	DLCI_OPENING,		/* Sending SABM not seen UA */
	DLCI_OPEN,		/* SABM/UA complete */
	DLCI_CLOSING,		/* Sending DISC not seen UA/DM */
};
/* …（同文件无关代码省略）… */
enum gsm_dlci_mode {
	DLCI_MODE_ABM,		/* Normal Asynchronous Balanced Mode */
	DLCI_MODE_ADM,		/* Asynchronous Disconnected Mode */
};
/* …（同文件无关代码省略）… */
struct gsm_dlci {
	struct gsm_mux *gsm;
	int addr;
	enum gsm_dlci_state state;
	struct mutex mutex;

	/* Link layer */
	enum gsm_dlci_mode mode;
	spinlock_t lock;	/* Protects the internal state */
	struct timer_list t1;	/* Retransmit timer for SABM and UA */
	int retries;
	/* Uplink tty if active */
	struct tty_port port;	/* The tty bound to this DLCI if there is one */
#define TX_SIZE		4096    /* Must be power of 2. */
	struct kfifo fifo;	/* Queue fifo for the DLCI */
	int adaption;		/* Adaption layer in use */
	int prev_adaption;
	u32 modem_rx;		/* Our incoming virtual modem lines */
	u32 modem_tx;		/* Our outgoing modem lines */
	unsigned int mtu;
	bool dead;		/* Refuse re-open */
	/* Configuration */
	u8 prio;		/* Priority */
	u8 ftype;		/* Frame type */
	u8 k;			/* Window size */
	/* Flow control */
	bool throttled;		/* Private copy of throttle state */
	bool constipated;	/* Throttle status for outgoing */
	/* Packetised I/O */
	struct sk_buff *skb;	/* Frame being sent */
	struct sk_buff_head skb_list;	/* Queued frames */
	/* Data handling callback */
	void (*data)(struct gsm_dlci *dlci, const u8 *data, int len);
	void (*prev_data)(struct gsm_dlci *dlci, const u8 *data, int len);
	struct net_device *net; /* network interface, if created */
};
/* …（同文件无关代码省略）… */
struct gsm_dlci_param_bits {
	u8 d_bits;
	u8 i_cl_bits;
	u8 p_bits;
	u8 t_bits;
	__le16 n_bits;
	u8 na_bits;
	u8 k_bits;
};
/* …（同文件无关代码省略）… */
#define PN_D_FIELD_DLCI		GENMASK(5, 0)
#define PN_I_CL_FIELD_FTYPE	GENMASK(3, 0)
#define PN_I_CL_FIELD_ADAPTION	GENMASK(7, 4)
#define PN_P_FIELD_PRIO		GENMASK(5, 0)
#define PN_T_FIELD_T1		GENMASK(7, 0)
#define PN_N_FIELD_N1		GENMASK(15, 0)
#define PN_NA_FIELD_N2		GENMASK(7, 0)
#define PN_K_FIELD_K		GENMASK(2, 0)
/* …（同文件无关代码省略）… */
#define NUM_DLCI		64
/* …（同文件无关代码省略）… */
struct gsm_control {
	u8 cmd;		/* Command we are issuing */
	u8 *data;	/* Data for the command in case we retransmit */
	int len;	/* Length of block for retransmission */
	int done;	/* Done flag */
	int error;	/* Error if any */
};
/* …（同文件无关代码省略）… */
enum gsm_encoding {
	GSM_BASIC_OPT,
	GSM_ADV_OPT,
};
/* …（同文件无关代码省略）… */
enum gsm_mux_state {
	GSM_SEARCH,
	GSM0_ADDRESS,
	GSM0_CONTROL,
	GSM0_LEN0,
	GSM0_LEN1,
	GSM0_DATA,
	GSM0_FCS,
	GSM0_SSOF,
	GSM1_START,
	GSM1_ADDRESS,
	GSM1_CONTROL,
	GSM1_DATA,
	GSM1_OVERRUN,
};
/* …（同文件无关代码省略）… */
struct gsm_mux {
	struct tty_struct *tty;		/* The tty our ldisc is bound to */
	spinlock_t lock;
	struct mutex mutex;
	unsigned int num;
	struct kref ref;

	/* Events on the GSM channel */
	wait_queue_head_t event;

	/* ldisc send work */
	struct work_struct tx_work;

	/* Bits for GSM mode decoding */

	/* Framing Layer */
	u8 *buf;
	enum gsm_mux_state state;
	unsigned int len;
	unsigned int address;
	unsigned int count;
	bool escape;
	enum gsm_encoding encoding;
	u8 control;
	u8 fcs;
	u8 *txframe;			/* TX framing buffer */

	/* Method for the receiver side */
	void (*receive)(struct gsm_mux *gsm, u8 ch);

	/* Link Layer */
	unsigned int mru;
	unsigned int mtu;
	int initiator;			/* Did we initiate connection */
	bool dead;			/* Has the mux been shut down */
	struct gsm_dlci *dlci[NUM_DLCI];
	int old_c_iflag;		/* termios c_iflag value before attach */
	bool constipated;		/* Asked by remote to shut up */
	bool has_devices;		/* Devices were registered */

	spinlock_t tx_lock;
	unsigned int tx_bytes;		/* TX data outstanding */
#define TX_THRESH_HI		8192
#define TX_THRESH_LO		2048
	struct list_head tx_ctrl_list;	/* Pending control packets */
	struct list_head tx_data_list;	/* Pending data packets */

	/* Control messages */
	struct timer_list kick_timer;	/* Kick TX queuing on timeout */
	struct timer_list t2_timer;	/* Retransmit timer for commands */
	int cretries;			/* Command retry counter */
	struct gsm_control *pending_cmd;/* Our current pending command */
	spinlock_t control_lock;	/* Protects the pending command */

	/* Keep-alive */
	struct timer_list ka_timer;	/* Keep-alive response timer */
	u8 ka_num;			/* Keep-alive match pattern */
	signed int ka_retries;		/* Keep-alive retry counter, -1 if not yet initialized */

	/* Configuration */
	int adaption;		/* 1 or 2 supported */
	u8 ftype;		/* UI or UIH */
	int t1, t2;		/* Timers in 1/100th of a sec */
	unsigned int t3;	/* Power wake-up timer in seconds. */
	int n2;			/* Retry count */
	u8 k;			/* Window size */
	bool wait_config;	/* Wait for configuration by ioctl before DLCI open */
	u32 keep_alive;		/* Control channel keep-alive in 10ms */

	/* Statistics (not currently exposed) */
	unsigned long bad_fcs;
	unsigned long malformed;
	unsigned long io_error;
	unsigned long open_error;
	unsigned long bad_size;
	unsigned long unsupported;
};
/* …（同文件无关代码省略）… */
#define MAX_MUX		4			/* 256 minors */
/* …（同文件无关代码省略）… */
#define CR			0x02
#define EA			0x01
#define	PF			0x10
/* …（同文件无关代码省略）… */
#define RR			0x01
#define UI			0x03
#define RNR			0x05
#define REJ			0x09
#define DM			0x0F
#define SABM			0x2F
#define DISC			0x43
#define UA			0x63
#define	UIH			0xEF
/* …（同文件无关代码省略）… */
#define CMD_NSC			0x09
#define CMD_TEST		0x11
#define CMD_PSC			0x21
#define CMD_RLS			0x29
#define CMD_FCOFF		0x31
#define CMD_PN			0x41
#define CMD_RPN			0x49
#define CMD_FCON		0x51
#define CMD_CLD			0x61
#define CMD_SNC			0x69
#define CMD_MSC			0x71
/* …（同文件无关代码省略）… */
#define MDM_FC			0x01
#define MDM_RTC			0x02
#define MDM_RTR			0x04
#define MDM_IC			0x20
#define MDM_DV			0x40
/* …（同文件无关代码省略）… */
#define GSM0_SOF		0xF9
#define GSM1_SOF		0x7E
#define GSM1_ESCAPE		0x7D
#define GSM1_ESCAPE_BITS	0x20
#define XON			0x11
#define XOFF			0x13
#define ISO_IEC_646_MASK	0x7F
/* …（同文件无关代码省略）… */
#define INIT_FCS	0xFF
/* …（同文件无关代码省略）… */

static void gsm_dlci_close(struct gsm_dlci *dlci);
static int gsmld_output(struct gsm_mux *gsm, u8 *data, int len);
static int gsm_modem_update(struct gsm_dlci *dlci, u8 brk);
static struct gsm_msg *gsm_data_alloc(struct gsm_mux *gsm, u8 addr, int len,
								u8 ctrl);
static int gsm_send_packet(struct gsm_mux *gsm, struct gsm_msg *msg);
static struct gsm_dlci *gsm_dlci_alloc(struct gsm_mux *gsm, int addr);
static void gsmld_write_trigger(struct gsm_mux *gsm);
static void gsmld_write_task(struct work_struct *work);
static int gsm_modem_send_initial_msc(struct gsm_dlci *dlci);

/**
 *	gsm_fcs_add	-	update FCS
/* …（同文件无关代码省略）… */
static inline u8 gsm_fcs_add_block(u8 fcs, u8 *c, int len)
{
	while (len--)
		fcs = gsm_fcs8[fcs ^ *c++];
	return fcs;
}
/* …（同文件无关代码省略）… */
static int gsm_read_ea(unsigned int *val, u8 c)
{
	/* Add the next 7 bits into the value */
	*val <<= 7;
	*val |= c >> 1;
	/* Was this the last byte of the EA 1 = yes*/
	return c & EA;
}
/* …（同文件无关代码省略）… */
static unsigned int gsm_read_ea_val(unsigned int *val, const u8 *data, int dlen)
{
	unsigned int len = 0;

	for (; dlen > 0; dlen--) {
		len++;
		if (gsm_read_ea(val, *data++))
			break;
	}
	return len;
}
/* …（同文件无关代码省略）… */
static u8 gsm_encode_modem(const struct gsm_dlci *dlci)
{
	u8 modembits = 0;
	/* FC is true flow control not modem bits */
	if (dlci->throttled)
		modembits |= MDM_FC;
	if (dlci->modem_tx & TIOCM_DTR)
		modembits |= MDM_RTC;
	if (dlci->modem_tx & TIOCM_RTS)
		modembits |= MDM_RTR;
	if (dlci->modem_tx & TIOCM_RI)
		modembits |= MDM_IC;
	if (dlci->modem_tx & TIOCM_CD || dlci->gsm->initiator)
		modembits |= MDM_DV;
	/* special mappings for passive side to operate as UE */
	if (dlci->modem_tx & TIOCM_OUT1)
		modembits |= MDM_IC;
	if (dlci->modem_tx & TIOCM_OUT2)
		modembits |= MDM_DV;
	return modembits;
}
/* …（同文件无关代码省略）… */
static void gsm_hex_dump_bytes(const char *fname, const u8 *data,
			       unsigned long len)
{
	char *prefix;

	if (!fname) {
		print_hex_dump(KERN_INFO, "", DUMP_PREFIX_NONE, 16, 1, data, len,
			       true);
		return;
	}

	prefix = kasprintf(GFP_ATOMIC, "%s: ", fname);
	if (!prefix)
		return;
	print_hex_dump(KERN_INFO, prefix, DUMP_PREFIX_OFFSET, 16, 1, data, len,
		       true);
	kfree(prefix);
}
/* …（同文件无关代码省略）… */
static int gsm_encode_params(const struct gsm_dlci *dlci,
			     struct gsm_dlci_param_bits *params)
{
	const struct gsm_mux *gsm = dlci->gsm;
	unsigned int i, cl;

	switch (dlci->ftype) {
	case UIH:
		i = 0; /* UIH */
		break;
	case UI:
		i = 1; /* UI */
		break;
	default:
		pr_debug("unsupported frame type %d\n", dlci->ftype);
		return -EINVAL;
	}

	switch (dlci->adaption) {
	case 1: /* Unstructured */
		cl = 0; /* convergence layer type 1 */
		break;
	case 2: /* Unstructured with modem bits. */
		cl = 1; /* convergence layer type 2 */
		break;
	default:
		pr_debug("unsupported adaption %d\n", dlci->adaption);
		return -EINVAL;
	}

	params->d_bits = FIELD_PREP(PN_D_FIELD_DLCI, dlci->addr);
	/* UIH, convergence layer type 1 */
	params->i_cl_bits = FIELD_PREP(PN_I_CL_FIELD_FTYPE, i) |
			    FIELD_PREP(PN_I_CL_FIELD_ADAPTION, cl);
	params->p_bits = FIELD_PREP(PN_P_FIELD_PRIO, dlci->prio);
	params->t_bits = FIELD_PREP(PN_T_FIELD_T1, gsm->t1);
	params->n_bits = cpu_to_le16(FIELD_PREP(PN_N_FIELD_N1, dlci->mtu));
	params->na_bits = FIELD_PREP(PN_NA_FIELD_N2, gsm->n2);
	params->k_bits = FIELD_PREP(PN_K_FIELD_K, dlci->k);

	return 0;
}
/* …（同文件无关代码省略）… */
static void gsm_print_packet(const char *hdr, int addr, int cr,
					u8 control, const u8 *data, int dlen)
{
	if (!(debug & DBG_DUMP))
		return;
	/* Only show user payload frames if debug & DBG_PAYLOAD */
	if (!(debug & DBG_PAYLOAD) && addr != 0)
		if ((control & ~PF) == UI || (control & ~PF) == UIH)
			return;

	pr_info("%s %d) %c: ", hdr, addr, "RC"[cr]);

	switch (control & ~PF) {
	case SABM:
		pr_cont("SABM");
		break;
	case UA:
		pr_cont("UA");
		break;
	case DISC:
		pr_cont("DISC");
		break;
	case DM:
		pr_cont("DM");
		break;
	case UI:
		pr_cont("UI");
		break;
	case UIH:
		pr_cont("UIH");
		break;
	default:
		if (!(control & 0x01)) {
			pr_cont("I N(S)%d N(R)%d",
				(control & 0x0E) >> 1, (control & 0xE0) >> 5);
		} else switch (control & 0x0F) {
			case RR:
				pr_cont("RR(%d)", (control & 0xE0) >> 5);
				break;
			case RNR:
				pr_cont("RNR(%d)", (control & 0xE0) >> 5);
				break;
			case REJ:
				pr_cont("REJ(%d)", (control & 0xE0) >> 5);
				break;
			default:
				pr_cont("[%02X]", control);
		}
	}

	if (control & PF)
		pr_cont("(P)");
	else
		pr_cont("(F)");

	gsm_hex_dump_bytes(NULL, data, dlen);
}
/* …（同文件无关代码省略）… */
static int gsm_stuff_frame(const u8 *input, u8 *output, int len)
{
	int olen = 0;
	while (len--) {
		if (*input == GSM1_SOF || *input == GSM1_ESCAPE
		    || (*input & ISO_IEC_646_MASK) == XON
		    || (*input & ISO_IEC_646_MASK) == XOFF) {
			*output++ = GSM1_ESCAPE;
			*output++ = *input++ ^ GSM1_ESCAPE_BITS;
			olen++;
		} else
			*output++ = *input++;
		olen++;
	}
	return olen;
}
/* …（同文件无关代码省略）… */
static int gsm_send(struct gsm_mux *gsm, int addr, int cr, int control)
{
	struct gsm_msg *msg;
	u8 *dp;
	int ocr;
	unsigned long flags;

	msg = gsm_data_alloc(gsm, addr, 0, control);
	if (!msg)
		return -ENOMEM;

	/* toggle C/R coding if not initiator */
	ocr = cr ^ (gsm->initiator ? 0 : 1);

	msg->data -= 3;
	dp = msg->data;
	*dp++ = (addr << 2) | (ocr << 1) | EA;
	*dp++ = control;

	if (gsm->encoding == GSM_BASIC_OPT)
		*dp++ = EA; /* Length of data = 0 */

	*dp = 0xFF - gsm_fcs_add_block(INIT_FCS, msg->data, dp - msg->data);
	msg->len = (dp - msg->data) + 1;

	gsm_print_packet("Q->", addr, cr, control, NULL, 0);

	spin_lock_irqsave(&gsm->tx_lock, flags);
	list_add_tail(&msg->list, &gsm->tx_ctrl_list);
	gsm->tx_bytes += msg->len;
	spin_unlock_irqrestore(&gsm->tx_lock, flags);
	gsmld_write_trigger(gsm);

	return 0;
}
/* …（同文件无关代码省略）… */
static void gsm_dlci_clear_queues(struct gsm_mux *gsm, struct gsm_dlci *dlci)
{
	struct gsm_msg *msg, *nmsg;
	int addr = dlci->addr;
	unsigned long flags;

	/* Clear DLCI write fifo first */
	spin_lock_irqsave(&dlci->lock, flags);
	kfifo_reset(&dlci->fifo);
	spin_unlock_irqrestore(&dlci->lock, flags);

	/* Clear data packets in MUX write queue */
	spin_lock_irqsave(&gsm->tx_lock, flags);
	list_for_each_entry_safe(msg, nmsg, &gsm->tx_data_list, list) {
		if (msg->addr != addr)
			continue;
		gsm->tx_bytes -= msg->len;
		list_del(&msg->list);
		kfree(msg);
	}
	spin_unlock_irqrestore(&gsm->tx_lock, flags);
}
/* …（同文件无关代码省略）… */
static inline void gsm_command(struct gsm_mux *gsm, int addr, int control)
{
	gsm_send(gsm, addr, 1, control);
}
/* …（同文件无关代码省略）… */
#define HDR_LEN		6	/* ADDR CTRL [LEN.2] DATA FCS */
/* …（同文件无关代码省略）… */
static struct gsm_msg *gsm_data_alloc(struct gsm_mux *gsm, u8 addr, int len,
								u8 ctrl)
{
	struct gsm_msg *m = kmalloc(sizeof(struct gsm_msg) + len + HDR_LEN,
								GFP_ATOMIC);
	if (m == NULL)
		return NULL;
	m->data = m->buffer + HDR_LEN - 1;	/* Allow for FCS */
	m->len = len;
	m->addr = addr;
	m->ctrl = ctrl;
	INIT_LIST_HEAD(&m->list);
	return m;
}
/* …（同文件无关代码省略）… */
static int gsm_send_packet(struct gsm_mux *gsm, struct gsm_msg *msg)
{
	int len, ret;


	if (gsm->encoding == GSM_BASIC_OPT) {
		gsm->txframe[0] = GSM0_SOF;
		memcpy(gsm->txframe + 1, msg->data, msg->len);
		gsm->txframe[msg->len + 1] = GSM0_SOF;
		len = msg->len + 2;
	} else {
		gsm->txframe[0] = GSM1_SOF;
		len = gsm_stuff_frame(msg->data, gsm->txframe + 1, msg->len);
		gsm->txframe[len + 1] = GSM1_SOF;
		len += 2;
	}

	if (debug & DBG_DATA)
		gsm_hex_dump_bytes(__func__, gsm->txframe, len);
	gsm_print_packet("-->", msg->addr, gsm->initiator, msg->ctrl, msg->data,
			 msg->len);

	ret = gsmld_output(gsm, gsm->txframe, len);
	if (ret <= 0)
		return ret;
	/* FIXME: Can eliminate one SOF in many more cases */
	gsm->tx_bytes -= msg->len;

	return 0;
}
/* …（同文件无关代码省略）… */
static bool gsm_is_flow_ctrl_msg(struct gsm_msg *msg)
{
	unsigned int cmd;

	if (msg->addr > 0)
		return false;

	switch (msg->ctrl & ~PF) {
	case UI:
	case UIH:
		cmd = 0;
		if (gsm_read_ea_val(&cmd, msg->data + 2, msg->len - 2) < 1)
			break;
		switch (cmd & ~PF) {
		case CMD_FCOFF:
		case CMD_FCON:
			return true;
		}
		break;
	}

	return false;
}
/* …（同文件无关代码省略）… */
static int gsm_data_kick(struct gsm_mux *gsm)
{
	struct gsm_msg *msg, *nmsg;
	struct gsm_dlci *dlci;
	int ret;

	clear_bit(TTY_DO_WRITE_WAKEUP, &gsm->tty->flags);

	/* Serialize control messages and control channel messages first */
	list_for_each_entry_safe(msg, nmsg, &gsm->tx_ctrl_list, list) {
		if (gsm->constipated && !gsm_is_flow_ctrl_msg(msg))
			continue;
		ret = gsm_send_packet(gsm, msg);
		switch (ret) {
		case -ENOSPC:
			return -ENOSPC;
		case -ENODEV:
			/* ldisc not open */
			gsm->tx_bytes -= msg->len;
			list_del(&msg->list);
			kfree(msg);
			continue;
		default:
			if (ret >= 0) {
				list_del(&msg->list);
				kfree(msg);
			}
			break;
		}
	}

	if (gsm->constipated)
		return -EAGAIN;

	/* Serialize other channels */
	if (list_empty(&gsm->tx_data_list))
		return 0;
	list_for_each_entry_safe(msg, nmsg, &gsm->tx_data_list, list) {
		dlci = gsm->dlci[msg->addr];
		/* Send only messages for DLCIs with valid state */
		if (dlci->state != DLCI_OPEN) {
			gsm->tx_bytes -= msg->len;
			list_del(&msg->list);
			kfree(msg);
			continue;
		}
		ret = gsm_send_packet(gsm, msg);
		switch (ret) {
		case -ENOSPC:
			return -ENOSPC;
		case -ENODEV:
			/* ldisc not open */
			gsm->tx_bytes -= msg->len;
			list_del(&msg->list);
			kfree(msg);
			continue;
		default:
			if (ret >= 0) {
				list_del(&msg->list);
				kfree(msg);
			}
			break;
		}
	}

	return 1;
}
/* …（同文件无关代码省略）… */
static void __gsm_data_queue(struct gsm_dlci *dlci, struct gsm_msg *msg)
{
	struct gsm_mux *gsm = dlci->gsm;
	u8 *dp = msg->data;
	u8 *fcs = dp + msg->len;

	/* Fill in the header */
	if (gsm->encoding == GSM_BASIC_OPT) {
		if (msg->len < 128)
			*--dp = (msg->len << 1) | EA;
		else {
			*--dp = (msg->len >> 7);	/* bits 7 - 15 */
			*--dp = (msg->len & 127) << 1;	/* bits 0 - 6 */
		}
	}

	*--dp = msg->ctrl;
	if (gsm->initiator)
		*--dp = (msg->addr << 2) | CR | EA;
	else
		*--dp = (msg->addr << 2) | EA;
	*fcs = gsm_fcs_add_block(INIT_FCS, dp , msg->data - dp);
	/* Ugly protocol layering violation */
	if (msg->ctrl == UI || msg->ctrl == (UI|PF))
		*fcs = gsm_fcs_add_block(*fcs, msg->data, msg->len);
	*fcs = 0xFF - *fcs;

	gsm_print_packet("Q> ", msg->addr, gsm->initiator, msg->ctrl,
							msg->data, msg->len);

	/* Move the header back and adjust the length, also allow for the FCS
	   now tacked on the end */
	msg->len += (msg->data - dp) + 1;
	msg->data = dp;

	/* Add to the actual output queue */
	switch (msg->ctrl & ~PF) {
	case UI:
	case UIH:
		if (msg->addr > 0) {
			list_add_tail(&msg->list, &gsm->tx_data_list);
			break;
		}
		fallthrough;
	default:
		list_add_tail(&msg->list, &gsm->tx_ctrl_list);
		break;
	}
	gsm->tx_bytes += msg->len;

	gsmld_write_trigger(gsm);
	mod_timer(&gsm->kick_timer, jiffies + 10 * gsm->t1 * HZ / 100);
}
/* …（同文件无关代码省略）… */
static void gsm_data_queue(struct gsm_dlci *dlci, struct gsm_msg *msg)
{
	unsigned long flags;
	spin_lock_irqsave(&dlci->gsm->tx_lock, flags);
	__gsm_data_queue(dlci, msg);
	spin_unlock_irqrestore(&dlci->gsm->tx_lock, flags);
}
/* …（同文件无关代码省略）… */
static int gsm_dlci_data_output(struct gsm_mux *gsm, struct gsm_dlci *dlci)
{
	struct gsm_msg *msg;
	u8 *dp;
	int h, len, size;

	/* for modem bits without break data */
	h = ((dlci->adaption == 1) ? 0 : 1);

	len = kfifo_len(&dlci->fifo);
	if (len == 0)
		return 0;

	/* MTU/MRU count only the data bits but watch adaption mode */
	if ((len + h) > dlci->mtu)
		len = dlci->mtu - h;

	size = len + h;

	msg = gsm_data_alloc(gsm, dlci->addr, size, dlci->ftype);
	if (!msg)
		return -ENOMEM;
	dp = msg->data;
	switch (dlci->adaption) {
	case 1: /* Unstructured */
		break;
	case 2: /* Unstructured with modem bits.
		 * Always one byte as we never send inline break data
		 */
		*dp++ = (gsm_encode_modem(dlci) << 1) | EA;
		break;
	default:
		pr_err("%s: unsupported adaption %d\n", __func__,
		       dlci->adaption);
		break;
	}

	WARN_ON(len != kfifo_out_locked(&dlci->fifo, dp, len,
		&dlci->lock));

	/* Notify upper layer about available send space. */
	tty_port_tty_wakeup(&dlci->port);

	__gsm_data_queue(dlci, msg);
	/* Bytes of data we used up */
	return size;
}
/* …（同文件无关代码省略）… */
static int gsm_dlci_data_output_framed(struct gsm_mux *gsm,
						struct gsm_dlci *dlci)
{
	struct gsm_msg *msg;
	u8 *dp;
	int len, size;
	int last = 0, first = 0;
	int overhead = 0;

	/* One byte per frame is used for B/F flags */
	if (dlci->adaption == 4)
		overhead = 1;

	/* dlci->skb is locked by tx_lock */
	if (dlci->skb == NULL) {
		dlci->skb = skb_dequeue_tail(&dlci->skb_list);
		if (dlci->skb == NULL)
			return 0;
		first = 1;
	}
	len = dlci->skb->len + overhead;

	/* MTU/MRU count only the data bits */
	if (len > dlci->mtu) {
		if (dlci->adaption == 3) {
			/* Over long frame, bin it */
			dev_kfree_skb_any(dlci->skb);
			dlci->skb = NULL;
			return 0;
		}
		len = dlci->mtu;
	} else
		last = 1;

	size = len + overhead;
	msg = gsm_data_alloc(gsm, dlci->addr, size, dlci->ftype);
	if (msg == NULL) {
		skb_queue_tail(&dlci->skb_list, dlci->skb);
		dlci->skb = NULL;
		return -ENOMEM;
	}
	dp = msg->data;

	if (dlci->adaption == 4) { /* Interruptible framed (Packetised Data) */
		/* Flag byte to carry the start/end info */
		*dp++ = last << 7 | first << 6 | 1;	/* EA */
		len--;
	}
	memcpy(dp, dlci->skb->data, len);
	skb_pull(dlci->skb, len);
	__gsm_data_queue(dlci, msg);
	if (last) {
		dev_kfree_skb_any(dlci->skb);
		dlci->skb = NULL;
	}
	return size;
}
/* …（同文件无关代码省略）… */
static int gsm_dlci_data_sweep(struct gsm_mux *gsm)
{
	/* Priority ordering: We should do priority with RR of the groups */
	int i, len, ret = 0;
	bool sent;
	struct gsm_dlci *dlci;

	while (gsm->tx_bytes < TX_THRESH_HI) {
		for (sent = false, i = 1; i < NUM_DLCI; i++) {
			dlci = gsm->dlci[i];
			/* skip unused or blocked channel */
			if (!dlci || dlci->constipated)
				continue;
			/* skip channels with invalid state */
			if (dlci->state != DLCI_OPEN)
				continue;
			/* count the sent data per adaption */
			if (dlci->adaption < 3 && !dlci->net)
				len = gsm_dlci_data_output(gsm, dlci);
			else
				len = gsm_dlci_data_output_framed(gsm, dlci);
			/* on error exit */
			if (len < 0)
				return ret;
			if (len > 0) {
				ret++;
				sent = true;
				/* The lower DLCs can starve the higher DLCs! */
				break;
			}
			/* try next */
		}
		if (!sent)
			break;
	}

	return ret;
}
/* …（同文件无关代码省略）… */
static void gsm_dlci_data_kick(struct gsm_dlci *dlci)
{
	unsigned long flags;
	int sweep;

	if (dlci->constipated)
		return;

	spin_lock_irqsave(&dlci->gsm->tx_lock, flags);
	/* If we have nothing running then we need to fire up */
	sweep = (dlci->gsm->tx_bytes < TX_THRESH_LO);
	if (dlci->gsm->tx_bytes == 0) {
		if (dlci->net)
			gsm_dlci_data_output_framed(dlci->gsm, dlci);
		else
			gsm_dlci_data_output(dlci->gsm, dlci);
	}
	if (sweep)
		gsm_dlci_data_sweep(dlci->gsm);
	spin_unlock_irqrestore(&dlci->gsm->tx_lock, flags);
}
/* …（同文件无关代码省略）… */
static int gsm_control_command(struct gsm_mux *gsm, int cmd, const u8 *data,
			       int dlen)
{
	struct gsm_msg *msg;
	struct gsm_dlci *dlci = gsm->dlci[0];

	msg = gsm_data_alloc(gsm, 0, dlen + 2, dlci->ftype);
	if (msg == NULL)
		return -ENOMEM;

	msg->data[0] = (cmd << 1) | CR | EA;	/* Set C/R */
	msg->data[1] = (dlen << 1) | EA;
	memcpy(msg->data + 2, data, dlen);
	gsm_data_queue(dlci, msg);

	return 0;
}
/* …（同文件无关代码省略）… */
static void gsm_control_reply(struct gsm_mux *gsm, int cmd, const u8 *data,
					int dlen)
{
	struct gsm_msg *msg;
	struct gsm_dlci *dlci = gsm->dlci[0];

	msg = gsm_data_alloc(gsm, 0, dlen + 2, dlci->ftype);
	if (msg == NULL)
		return;
	msg->data[0] = (cmd & 0xFE) << 1 | EA;	/* Clear C/R */
	msg->data[1] = (dlen << 1) | EA;
	memcpy(msg->data + 2, data, dlen);
	gsm_data_queue(dlci, msg);
}
/* …（同文件无关代码省略）… */
static void gsm_process_modem(struct tty_struct *tty, struct gsm_dlci *dlci,
							u32 modem, int slen)
{
	int  mlines = 0;
	u8 brk = 0;
	int fc;

	/* The modem status command can either contain one octet (V.24 signals)
	 * or two octets (V.24 signals + break signals). This is specified in
	 * section 5.4.6.3.7 of the 07.10 mux spec.
	 */

	if (slen == 1)
		modem = modem & 0x7f;
	else {
		brk = modem & 0x7f;
		modem = (modem >> 7) & 0x7f;
	}

	/* Flow control/ready to communicate */
	fc = (modem & MDM_FC) || !(modem & MDM_RTR);
	if (fc && !dlci->constipated) {
		/* Need to throttle our output on this device */
		dlci->constipated = true;
	} else if (!fc && dlci->constipated) {
		dlci->constipated = false;
		gsm_dlci_data_kick(dlci);
	}

	/* Map modem bits */
	if (modem & MDM_RTC)
		mlines |= TIOCM_DSR | TIOCM_DTR;
	if (modem & MDM_RTR)
		mlines |= TIOCM_RTS | TIOCM_CTS;
	if (modem & MDM_IC)
		mlines |= TIOCM_RI;
	if (modem & MDM_DV)
		mlines |= TIOCM_CD;

	/* Carrier drop -> hangup */
	if (tty) {
		if ((mlines & TIOCM_CD) == 0 && (dlci->modem_rx & TIOCM_CD))
			if (!C_CLOCAL(tty))
				tty_hangup(tty);
	}
	if (brk & 0x01)
		tty_insert_flip_char(&dlci->port, 0, TTY_BREAK);
	dlci->modem_rx = mlines;
	wake_up_interruptible(&dlci->gsm->event);
}
/* …（同文件无关代码省略）… */
static int gsm_process_negotiation(struct gsm_mux *gsm, unsigned int addr,
				   unsigned int cr,
				   const struct gsm_dlci_param_bits *params)
{
	struct gsm_dlci *dlci = gsm->dlci[addr];
	unsigned int ftype, i, adaption, prio, n1, k;

	i = FIELD_GET(PN_I_CL_FIELD_FTYPE, params->i_cl_bits);
	adaption = FIELD_GET(PN_I_CL_FIELD_ADAPTION, params->i_cl_bits) + 1;
	prio = FIELD_GET(PN_P_FIELD_PRIO, params->p_bits);
	n1 = FIELD_GET(PN_N_FIELD_N1, get_unaligned_le16(&params->n_bits));
	k = FIELD_GET(PN_K_FIELD_K, params->k_bits);

	if (n1 < MIN_MTU) {
		if (debug & DBG_ERRORS)
			pr_info("%s N1 out of range in PN\n", __func__);
		return -EINVAL;
	}

	switch (i) {
	case 0x00:
		ftype = UIH;
		break;
	case 0x01:
		ftype = UI;
		break;
	case 0x02: /* I frames are not supported */
		if (debug & DBG_ERRORS)
			pr_info("%s unsupported I frame request in PN\n",
				__func__);
		gsm->unsupported++;
		return -EINVAL;
	default:
		if (debug & DBG_ERRORS)
			pr_info("%s i out of range in PN\n", __func__);
		return -EINVAL;
	}

	if (!cr && gsm->initiator) {
		if (adaption != dlci->adaption) {
			if (debug & DBG_ERRORS)
				pr_info("%s invalid adaption %d in PN\n",
					__func__, adaption);
			return -EINVAL;
		}
		if (prio != dlci->prio) {
			if (debug & DBG_ERRORS)
				pr_info("%s invalid priority %d in PN",
					__func__, prio);
			return -EINVAL;
		}
		if (n1 > gsm->mru || n1 > dlci->mtu) {
			/* We requested a frame size but the other party wants
			 * to send larger frames. The standard allows only a
			 * smaller response value than requested (5.4.6.3.1).
			 */
			if (debug & DBG_ERRORS)
				pr_info("%s invalid N1 %d in PN\n", __func__,
					n1);
			return -EINVAL;
		}
		dlci->mtu = n1;
		if (ftype != dlci->ftype) {
			if (debug & DBG_ERRORS)
				pr_info("%s invalid i %d in PN\n", __func__, i);
			return -EINVAL;
		}
		if (ftype != UI && ftype != UIH && k > dlci->k) {
			if (debug & DBG_ERRORS)
				pr_info("%s invalid k %d in PN\n", __func__, k);
			return -EINVAL;
		}
		dlci->k = k;
	} else if (cr && !gsm->initiator) {
		/* Only convergence layer type 1 and 2 are supported. */
		if (adaption != 1 && adaption != 2) {
			if (debug & DBG_ERRORS)
				pr_info("%s invalid adaption %d in PN\n",
					__func__, adaption);
			return -EINVAL;
		}
		dlci->adaption = adaption;
		if (n1 > gsm->mru) {
			/* Propose a smaller value */
			dlci->mtu = gsm->mru;
		} else if (n1 > MAX_MTU) {
			/* Propose a smaller value */
			dlci->mtu = MAX_MTU;
		} else {
			dlci->mtu = n1;
		}
		dlci->prio = prio;
		dlci->ftype = ftype;
		dlci->k = k;
	} else {
		return -EINVAL;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static void gsm_control_modem(struct gsm_mux *gsm, const u8 *data, int clen)
{
	unsigned int addr = 0;
	unsigned int modem = 0;
	struct gsm_dlci *dlci;
	int len = clen;
	int cl = clen;
	const u8 *dp = data;
	struct tty_struct *tty;

	len = gsm_read_ea_val(&addr, data, cl);
	if (len < 1)
		return;

	addr >>= 1;
	/* Closed port, or invalid ? */
	if (addr == 0 || addr >= NUM_DLCI || gsm->dlci[addr] == NULL)
		return;
	dlci = gsm->dlci[addr];

	/* Must be at least one byte following the EA */
	if ((cl - len) < 1)
		return;

	dp += len;
	cl -= len;

	/* get the modem status */
	len = gsm_read_ea_val(&modem, dp, cl);
	if (len < 1)
		return;

	tty = tty_port_tty_get(&dlci->port);
	gsm_process_modem(tty, dlci, modem, cl);
	if (tty) {
		tty_wakeup(tty);
		tty_kref_put(tty);
	}
	gsm_control_reply(gsm, CMD_MSC, data, clen);
}
/* …（同文件无关代码省略）… */
static void gsm_control_negotiation(struct gsm_mux *gsm, unsigned int cr,
				    const u8 *data, unsigned int dlen)
{
	unsigned int addr;
	struct gsm_dlci_param_bits pn_reply;
	struct gsm_dlci *dlci;
	struct gsm_dlci_param_bits *params;

	if (dlen < sizeof(struct gsm_dlci_param_bits)) {
		gsm->open_error++;
		return;
	}

	/* Invalid DLCI? */
	params = (struct gsm_dlci_param_bits *)data;
	addr = FIELD_GET(PN_D_FIELD_DLCI, params->d_bits);
	if (addr == 0 || addr >= NUM_DLCI || !gsm->dlci[addr]) {
		gsm->open_error++;
		return;
	}
	dlci = gsm->dlci[addr];

	/* Too late for parameter negotiation? */
	if ((!cr && dlci->state == DLCI_OPENING) || dlci->state == DLCI_OPEN) {
		gsm->open_error++;
		return;
	}

	/* Process the received parameters */
	if (gsm_process_negotiation(gsm, addr, cr, params) != 0) {
		/* Negotiation failed. Close the link. */
		if (debug & DBG_ERRORS)
			pr_info("%s PN failed\n", __func__);
		gsm->open_error++;
		gsm_dlci_close(dlci);
		return;
	}

	if (cr) {
		/* Reply command with accepted parameters. */
		if (gsm_encode_params(dlci, &pn_reply) == 0)
			gsm_control_reply(gsm, CMD_PN, (const u8 *)&pn_reply,
					  sizeof(pn_reply));
		else if (debug & DBG_ERRORS)
			pr_info("%s PN invalid\n", __func__);
	} else if (dlci->state == DLCI_CONFIGURE) {
		/* Proceed with link setup by sending SABM before UA */
		dlci->state = DLCI_OPENING;
		gsm_command(gsm, dlci->addr, SABM|PF);
		mod_timer(&dlci->t1, jiffies + gsm->t1 * HZ / 100);
	} else {
		if (debug & DBG_ERRORS)
			pr_info("%s PN in invalid state\n", __func__);
		gsm->open_error++;
	}
}
/* …（同文件无关代码省略）… */
static void gsm_control_rls(struct gsm_mux *gsm, const u8 *data, int clen)
{
	struct tty_port *port;
	unsigned int addr = 0;
	u8 bits;
	int len = clen;
	const u8 *dp = data;

	while (gsm_read_ea(&addr, *dp++) == 0) {
		len--;
		if (len == 0)
			return;
	}
	/* Must be at least one byte following ea */
	len--;
	if (len <= 0)
		return;
	addr >>= 1;
	/* Closed port, or invalid ? */
	if (addr == 0 || addr >= NUM_DLCI || gsm->dlci[addr] == NULL)
		return;
	/* No error ? */
	bits = *dp;
	if ((bits & 1) == 0)
		return;

	port = &gsm->dlci[addr]->port;

	if (bits & 2)
		tty_insert_flip_char(port, 0, TTY_OVERRUN);
	if (bits & 4)
		tty_insert_flip_char(port, 0, TTY_PARITY);
	if (bits & 8)
		tty_insert_flip_char(port, 0, TTY_FRAME);

	tty_flip_buffer_push(port);

	gsm_control_reply(gsm, CMD_RLS, data, clen);
}
/* …（同文件无关代码省略）… */
static void gsm_control_message(struct gsm_mux *gsm, unsigned int command,
						const u8 *data, int clen)
{
	u8 buf[1];

	switch (command) {
	case CMD_CLD: {
		struct gsm_dlci *dlci = gsm->dlci[0];
		/* Modem wishes to close down */
		if (dlci) {
			dlci->dead = true;
			gsm->dead = true;
			gsm_dlci_begin_close(dlci);
		}
		}
		break;
	case CMD_TEST:
		/* Modem wishes to test, reply with the data */
		gsm_control_reply(gsm, CMD_TEST, data, clen);
		break;
	case CMD_FCON:
		/* Modem can accept data again */
		gsm->constipated = false;
		gsm_control_reply(gsm, CMD_FCON, NULL, 0);
		/* Kick the link in case it is idling */
		gsmld_write_trigger(gsm);
		break;
	case CMD_FCOFF:
		/* Modem wants us to STFU */
		gsm->constipated = true;
		gsm_control_reply(gsm, CMD_FCOFF, NULL, 0);
		break;
	case CMD_MSC:
		/* Out of band modem line change indicator for a DLCI */
		gsm_control_modem(gsm, data, clen);
		break;
	case CMD_RLS:
		/* Out of band error reception for a DLCI */
		gsm_control_rls(gsm, data, clen);
		break;
	case CMD_PSC:
		/* Modem wishes to enter power saving state */
		gsm_control_reply(gsm, CMD_PSC, NULL, 0);
		break;
		/* Optional commands */
	case CMD_PN:
		/* Modem sends a parameter negotiation command */
		gsm_control_negotiation(gsm, 1, data, clen);
		break;
		/* Optional unsupported commands */
	case CMD_RPN:	/* Remote port negotiation */
	case CMD_SNC:	/* Service negotiation command */
		gsm->unsupported++;
		fallthrough;
	default:
		/* Reply to bad commands with an NSC */
		buf[0] = command;
		gsm_control_reply(gsm, CMD_NSC, buf, 1);
		break;
	}
}
/* …（同文件无关代码省略）… */
static void gsm_control_response(struct gsm_mux *gsm, unsigned int command,
						const u8 *data, int clen)
{
	struct gsm_control *ctrl;
	struct gsm_dlci *dlci;
	unsigned long flags;

	spin_lock_irqsave(&gsm->control_lock, flags);

	ctrl = gsm->pending_cmd;
	dlci = gsm->dlci[0];
	command |= 1;
	/* Does the reply match our command */
	if (ctrl != NULL && (command == ctrl->cmd || command == CMD_NSC)) {
		/* Our command was replied to, kill the retry timer */
		timer_delete(&gsm->t2_timer);
		gsm->pending_cmd = NULL;
		/* Rejected by the other end */
		if (command == CMD_NSC)
			ctrl->error = -EOPNOTSUPP;
		ctrl->done = 1;
		wake_up(&gsm->event);
	/* Or did we receive the PN response to our PN command */
	} else if (command == CMD_PN) {
		gsm_control_negotiation(gsm, 0, data, clen);
	/* Or did we receive the TEST response to our TEST command */
	} else if (command == CMD_TEST && clen == 1 && *data == gsm->ka_num) {
		gsm->ka_retries = -1; /* trigger new keep-alive message */
		if (dlci && !dlci->dead)
			mod_timer(&gsm->ka_timer, jiffies + gsm->keep_alive * HZ / 100);
	}
	spin_unlock_irqrestore(&gsm->control_lock, flags);
}
/* …（同文件无关代码省略）… */
static void gsm_control_transmit(struct gsm_mux *gsm, struct gsm_control *ctrl)
{
	gsm_control_command(gsm, ctrl->cmd, ctrl->data, ctrl->len);
}
/* …（同文件无关代码省略）… */
static struct gsm_control *gsm_control_send(struct gsm_mux *gsm,
		unsigned int command, u8 *data, int clen)
{
	struct gsm_control *ctrl = kzalloc_obj(struct gsm_control, GFP_ATOMIC);
	unsigned long flags;
	if (ctrl == NULL)
		return NULL;
retry:
	wait_event(gsm->event, gsm->pending_cmd == NULL);
	spin_lock_irqsave(&gsm->control_lock, flags);
	if (gsm->pending_cmd != NULL) {
		spin_unlock_irqrestore(&gsm->control_lock, flags);
		goto retry;
	}
	ctrl->cmd = command;
	ctrl->data = data;
	ctrl->len = clen;
	gsm->pending_cmd = ctrl;

	/* If DLCI0 is in ADM mode skip retries, it won't respond */
	if (gsm->dlci[0]->mode == DLCI_MODE_ADM)
		gsm->cretries = 0;
	else
		gsm->cretries = gsm->n2;

	mod_timer(&gsm->t2_timer, jiffies + gsm->t2 * HZ / 100);
	gsm_control_transmit(gsm, ctrl);
	spin_unlock_irqrestore(&gsm->control_lock, flags);
	return ctrl;
}
/* …（同文件无关代码省略）… */
static int gsm_control_wait(struct gsm_mux *gsm, struct gsm_control *control)
{
	int err;
	wait_event(gsm->event, control->done == 1);
	err = control->error;
	kfree(control);
	return err;
}
/* …（同文件无关代码省略）… */
static void gsm_dlci_close(struct gsm_dlci *dlci)
{
	timer_delete(&dlci->t1);
	if (debug & DBG_ERRORS)
		pr_debug("DLCI %d goes closed.\n", dlci->addr);
	dlci->state = DLCI_CLOSED;
	/* Prevent us from sending data before the link is up again */
	dlci->constipated = true;
	if (dlci->addr != 0) {
		tty_port_tty_hangup(&dlci->port, false);
		gsm_dlci_clear_queues(dlci->gsm, dlci);
		/* Ensure that gsmtty_open() can return. */
		tty_port_set_initialized(&dlci->port, false);
		wake_up_interruptible(&dlci->port.open_wait);
	} else {
		timer_delete(&dlci->gsm->ka_timer);
		dlci->gsm->dead = true;
	}
	/* A DLCI 0 close is a MUX termination so we need to kick that
	   back to userspace somehow */
	gsm_dlci_data_kick(dlci);
	wake_up_all(&dlci->gsm->event);
}
/* …（同文件无关代码省略）… */
static void gsm_dlci_open(struct gsm_dlci *dlci)
{
	struct gsm_mux *gsm = dlci->gsm;

	/* Note that SABM UA .. SABM UA first UA lost can mean that we go
	   open -> open */
	timer_delete(&dlci->t1);
	/* This will let a tty open continue */
	dlci->state = DLCI_OPEN;
	dlci->constipated = false;
	if (debug & DBG_ERRORS)
		pr_debug("DLCI %d goes open.\n", dlci->addr);
	/* Send current modem state */
	if (dlci->addr) {
		gsm_modem_send_initial_msc(dlci);
	} else {
		/* Start keep-alive control */
		gsm->ka_num = 0;
		gsm->ka_retries = -1;
		mod_timer(&gsm->ka_timer,
			  jiffies + gsm->keep_alive * HZ / 100);
	}
	gsm_dlci_data_kick(dlci);
	wake_up(&dlci->gsm->event);
}
/* …（同文件无关代码省略）… */
static int gsm_dlci_negotiate(struct gsm_dlci *dlci)
{
	struct gsm_mux *gsm = dlci->gsm;
	struct gsm_dlci_param_bits params;
	int ret;

	ret = gsm_encode_params(dlci, &params);
	if (ret != 0)
		return ret;

	/* We cannot asynchronous wait for the command response with
	 * gsm_command() and gsm_control_wait() at this point.
	 */
	ret = gsm_control_command(gsm, CMD_PN, (const u8 *)&params,
				  sizeof(params));

	return ret;
}
/* …（同文件无关代码省略）… */
static void gsm_dlci_t1(struct timer_list *t)
{
	struct gsm_dlci *dlci = timer_container_of(dlci, t, t1);
	struct gsm_mux *gsm = dlci->gsm;

	switch (dlci->state) {
	case DLCI_CONFIGURE:
		if (dlci->retries && gsm_dlci_negotiate(dlci) == 0) {
			dlci->retries--;
			mod_timer(&dlci->t1, jiffies + gsm->t1 * HZ / 100);
		} else {
			gsm->open_error++;
			gsm_dlci_begin_close(dlci); /* prevent half open link */
		}
		break;
	case DLCI_OPENING:
		if (!dlci->addr && gsm->control == (DM | PF)) {
			if (debug & DBG_ERRORS)
				pr_info("DLCI 0 opening in ADM mode.\n");
			dlci->mode = DLCI_MODE_ADM;
			gsm_dlci_open(dlci);
		} else if (dlci->retries) {
			if (!dlci->addr || !gsm->dlci[0] ||
			    gsm->dlci[0]->state != DLCI_OPENING) {
				dlci->retries--;
				gsm_command(dlci->gsm, dlci->addr, SABM|PF);
			}

			mod_timer(&dlci->t1, jiffies + gsm->t1 * HZ / 100);
		} else {
			gsm->open_error++;
			gsm_dlci_begin_close(dlci); /* prevent half open link */
		}

		break;
	case DLCI_CLOSING:
		if (dlci->retries) {
			dlci->retries--;
			gsm_command(dlci->gsm, dlci->addr, DISC|PF);
			mod_timer(&dlci->t1, jiffies + gsm->t1 * HZ / 100);
		} else
			gsm_dlci_close(dlci);
		break;
	default:
		pr_debug("%s: unhandled state: %d\n", __func__, dlci->state);
		break;
	}
}
/* …（同文件无关代码省略）… */
static void gsm_dlci_begin_close(struct gsm_dlci *dlci)
{
	struct gsm_mux *gsm = dlci->gsm;
	if (dlci->state == DLCI_CLOSED || dlci->state == DLCI_CLOSING)
		return;
	dlci->retries = gsm->n2;
	dlci->state = DLCI_CLOSING;
	gsm_command(dlci->gsm, dlci->addr, DISC|PF);
	mod_timer(&dlci->t1, jiffies + gsm->t1 * HZ / 100);
	wake_up_interruptible(&gsm->event);
}
/* …（同文件无关代码省略）… */
static void gsm_dlci_data(struct gsm_dlci *dlci, const u8 *data, int clen)
{
	/* krefs .. */
	struct tty_port *port = &dlci->port;
	struct tty_struct *tty;
	unsigned int modem = 0;
	int len;

	if (debug & DBG_TTY)
		pr_debug("%d bytes for tty\n", clen);
	switch (dlci->adaption)  {
	/* Unsupported types */
	case 4:		/* Packetised interruptible data */
		break;
	case 3:		/* Packetised uininterruptible voice/data */
		break;
	case 2:		/* Asynchronous serial with line state in each frame */
		len = gsm_read_ea_val(&modem, data, clen);
		if (len < 1)
			return;
		tty = tty_port_tty_get(port);
		if (tty) {
			gsm_process_modem(tty, dlci, modem, len);
			tty_wakeup(tty);
			tty_kref_put(tty);
		}
		/* Skip processed modem data */
		data += len;
		clen -= len;
		fallthrough;
	case 1:		/* Line state will go via DLCI 0 controls only */
	default:
		tty_insert_flip_string(port, data, clen);
		tty_flip_buffer_push(port);
	}
}
/* …（同文件无关代码省略）… */
static void gsm_dlci_command(struct gsm_dlci *dlci, const u8 *data, int len)
{
	/* See what command is involved */
	unsigned int command = 0;
	unsigned int clen = 0;
	unsigned int dlen;

	/* read the command */
	dlen = gsm_read_ea_val(&command, data, len);
	len -= dlen;
	data += dlen;

	/* read any control data */
	dlen = gsm_read_ea_val(&clen, data, len);
	len -= dlen;
	data += dlen;

	/* Malformed command? */
	if (clen > len) {
		dlci->gsm->malformed++;
		return;
	}

	if (command & 1)
		gsm_control_message(dlci->gsm, command, data, clen);
	else
		gsm_control_response(dlci->gsm, command, data, clen);
}
/* …（同文件无关代码省略）… */
static struct gsm_dlci *gsm_dlci_alloc(struct gsm_mux *gsm, int addr)
{
	struct gsm_dlci *dlci = kzalloc_obj(struct gsm_dlci, GFP_ATOMIC);
	if (dlci == NULL)
		return NULL;
	spin_lock_init(&dlci->lock);
	mutex_init(&dlci->mutex);
	if (kfifo_alloc(&dlci->fifo, TX_SIZE, GFP_KERNEL) < 0) {
		kfree(dlci);
		return NULL;
	}

	skb_queue_head_init(&dlci->skb_list);
	timer_setup(&dlci->t1, gsm_dlci_t1, 0);
	tty_port_init(&dlci->port);
	dlci->port.ops = &gsm_port_ops;
	dlci->gsm = gsm;
	dlci->addr = addr;
	dlci->adaption = gsm->adaption;
	dlci->mtu = gsm->mtu;
	if (addr == 0)
		dlci->prio = 0;
	else
		dlci->prio = roundup(addr + 1, 8) - 1;
	dlci->ftype = gsm->ftype;
	dlci->k = gsm->k;
	dlci->state = DLCI_CLOSED;
	if (addr) {
		dlci->data = gsm_dlci_data;
		/* Prevent us from sending data before the link is up */
		dlci->constipated = true;
	} else {
		dlci->data = gsm_dlci_command;
	}
	gsm->dlci[addr] = dlci;
	return dlci;
}
/* …（同文件无关代码省略）… */
static inline void dlci_put(struct gsm_dlci *dlci)
{
	tty_port_put(&dlci->port);
}
/* …（同文件无关代码省略）… */
static void gsm_free_mux(struct gsm_mux *gsm)
{
	int i;

	for (i = 0; i < MAX_MUX; i++) {
		if (gsm == gsm_mux[i]) {
			gsm_mux[i] = NULL;
			break;
		}
	}
	mutex_destroy(&gsm->mutex);
	kfree(gsm->txframe);
	kfree(gsm->buf);
	kfree(gsm);
}
/* …（同文件无关代码省略）… */
static void gsm_free_muxr(struct kref *ref)
{
	struct gsm_mux *gsm = container_of(ref, struct gsm_mux, ref);
	gsm_free_mux(gsm);
}
/* …（同文件无关代码省略）… */
static inline void mux_put(struct gsm_mux *gsm)
{
	unsigned long flags;

	spin_lock_irqsave(&gsm_mux_lock, flags);
	kref_put(&gsm->ref, gsm_free_muxr);
	spin_unlock_irqrestore(&gsm_mux_lock, flags);
}
/* …（同文件无关代码省略）… */
static int gsmld_output(struct gsm_mux *gsm, u8 *data, int len)
{
	if (tty_write_room(gsm->tty) < len) {
		set_bit(TTY_DO_WRITE_WAKEUP, &gsm->tty->flags);
		return -ENOSPC;
	}
	if (debug & DBG_DATA)
		gsm_hex_dump_bytes(__func__, data, len);
	return gsm->tty->ops->write(gsm->tty, data, len);
}
/* …（同文件无关代码省略）… */
static void gsmld_write_trigger(struct gsm_mux *gsm)
{
	if (!gsm || !gsm->dlci[0] || gsm->dlci[0]->dead)
		return;
	schedule_work(&gsm->tx_work);
}
/* …（同文件无关代码省略）… */
static void gsmld_write_task(struct work_struct *work)
{
	struct gsm_mux *gsm = container_of(work, struct gsm_mux, tx_work);
	unsigned long flags;
	int i, ret;

	/* All outstanding control channel and control messages and one data
	 * frame is sent.
	 */
	ret = -ENODEV;
	spin_lock_irqsave(&gsm->tx_lock, flags);
	if (gsm->tty)
		ret = gsm_data_kick(gsm);
	spin_unlock_irqrestore(&gsm->tx_lock, flags);

	if (ret >= 0)
		for (i = 0; i < NUM_DLCI; i++)
			if (gsm->dlci[i])
				tty_port_tty_wakeup(&gsm->dlci[i]->port);
}
/* …（同文件无关代码省略）… */
 *	gsm_modem_upd_via_msc	-	send modem bits via control frame
 *	@dlci: channel
 *	@brk: break signal
 */

static int gsm_modem_upd_via_msc(struct gsm_dlci *dlci, u8 brk)
{
	u8 modembits[3];
	struct gsm_control *ctrl;
/* …（同文件无关代码省略）… */
		modembits[2] = (brk << 4) | 2 | EA; /* Length, Break, EA */
		len++;
	}
	ctrl = gsm_control_send(dlci->gsm, CMD_MSC, modembits, len);
	if (ctrl == NULL)
		return -ENOMEM;
	return gsm_control_wait(dlci->gsm, ctrl);
}

/**
 * gsm_modem_send_initial_msc - Send initial modem status message
 *
 * @dlci: channel
 *
 * Send an initial MSC message after DLCI open to set the initial
 * modem status lines. This is only done for basic mode.
 * Does not wait for a response as we cannot block the input queue
 * processing.
 */
static int gsm_modem_send_initial_msc(struct gsm_dlci *dlci)
{
	u8 modembits[2];

	if (dlci->adaption != 1 || dlci->gsm->encoding != GSM_BASIC_OPT)
		return 0;

	modembits[0] = (dlci->addr << 2) | 2 | EA; /* DLCI, Valid, EA */
	modembits[1] = (gsm_encode_modem(dlci) << 1) | EA;
	return gsm_control_command(dlci->gsm, CMD_MSC, (const u8 *)&modembits, 2);
}

/**
 *	gsm_modem_update	-	send modem status line state
 *	@dlci: channel
 *	@brk: break signal
 */

static int gsm_modem_update(struct gsm_dlci *dlci, u8 brk)
{
	if (dlci->gsm->dead)
		return -EL2HLT;
/* …（同文件无关代码省略）… */
		return 0;
	} else if (dlci->gsm->encoding == GSM_BASIC_OPT) {
		/* Send as MSC control message. */
		return gsm_modem_upd_via_msc(dlci, brk);
	}

	/* Modem status lines are not supported. */
/* …（同文件无关代码省略）… */
		modem_tx &= ~(TIOCM_DTR | TIOCM_RTS);
	if (modem_tx != dlci->modem_tx) {
		dlci->modem_tx = modem_tx;
		gsm_modem_update(dlci, 0);
	}
}

/* …（同文件无关代码省略）… */

	if (modem_tx != dlci->modem_tx) {
		dlci->modem_tx = modem_tx;
		return gsm_modem_update(dlci, 0);
	}
	return 0;
}
/* …（同文件无关代码省略）… */
		dlci->modem_tx &= ~TIOCM_RTS;
	dlci->throttled = true;
	/* Send an MSC with RTS cleared */
	gsm_modem_update(dlci, 0);
}

static void gsmtty_unthrottle(struct tty_struct *tty)
{
	struct gsm_dlci *dlci = tty->driver_data;
	if (dlci->state == DLCI_CLOSED)
		return;
	if (C_CRTSCTS(tty))
		dlci->modem_tx |= TIOCM_RTS;
	dlci->throttled = false;
	/* Send an MSC with RTS set */
	gsm_modem_update(dlci, 0);
}

static int gsmtty_break_ctl(struct tty_struct *tty, int state)
{
	struct gsm_dlci *dlci = tty->driver_data;
	int encode = 0;	/* Off */
	if (dlci->state == DLCI_CLOSED)
		return -EINVAL;

	if (state == -1)	/* "On indefinitely" - we can't encode this
				    properly */
		encode = 0x0F;
	else if (state > 0) {
		encode = state / 200;	/* mS to encoding */
		if (encode > 0x0F)
			encode = 0x0F;	/* Best effort */
	}
	return gsm_modem_update(dlci, encode);
}

static void gsmtty_cleanup(struct tty_struct *tty)
{
	struct gsm_dlci *dlci = tty->driver_data;
	struct gsm_mux *gsm = dlci->gsm;

	dlci_put(dlci);
	dlci_put(gsm->dlci[0]);
	mux_put(gsm);
}
