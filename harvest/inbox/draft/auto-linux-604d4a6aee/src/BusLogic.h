// AUTO-DRAFT from torvalds/linux PR #a41e364995bbcfa627418a70fe913c35b7510728
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
#define PACKED __attribute__((packed))
/* …（同文件无关代码省略）… */
#define BLOGIC_MAXDEV			16
/* …（同文件无关代码省略）… */
#define BLOGIC_SG_LIMIT		128
/* …（同文件无关代码省略）… */
#define BLOGIC_MSGBUF_SIZE		9700
/* …（同文件无关代码省略）… */
enum blogic_adapter_type {
	BLOGIC_MULTIMASTER = 1,
	BLOGIC_FLASHPOINT = 2
} PACKED;
/* …（同文件无关代码省略）… */
enum blogic_adapter_bus_type {
	BLOGIC_UNKNOWN_BUS = 0,
	BLOGIC_ISA_BUS = 1,
	BLOGIC_EISA_BUS = 2,
	BLOGIC_PCI_BUS = 3,
	BLOGIC_VESA_BUS = 4,
	BLOGIC_MCA_BUS = 5
} PACKED;
/* …（同文件无关代码省略）… */
struct blogic_byte_count {
	unsigned int units;
	unsigned int billions;
};
/* …（同文件无关代码省略）… */
enum blogic_action {
	BLOGIC_OUTBOX_FREE = 0x00,
	BLOGIC_MBOX_START = 0x01,
	BLOGIC_MBOX_ABORT = 0x02
} PACKED;
/* …（同文件无关代码省略）… */
enum blogic_cmplt_code {
	BLOGIC_INBOX_FREE = 0x00,
	BLOGIC_CMD_COMPLETE_GOOD = 0x01,
	BLOGIC_CMD_ABORT_BY_HOST = 0x02,
	BLOGIC_CMD_NOTFOUND = 0x03,
	BLOGIC_CMD_COMPLETE_ERROR = 0x04,
	BLOGIC_INVALID_CCB = 0x05
} PACKED;
/* …（同文件无关代码省略）… */
enum blogic_ccb_opcode {
	BLOGIC_INITIATOR_CCB = 0x00,
	BLOGIC_TGT_CCB = 0x01,
	BLOGIC_INITIATOR_CCB_SG = 0x02,
	BLOGIC_INITIATOR_CCBB_RESIDUAL = 0x03,
	BLOGIC_INITIATOR_CCB_SG_RESIDUAL = 0x04,
	BLOGIC_BDR = 0x81
} PACKED;
/* …（同文件无关代码省略）… */
enum blogic_datadir {
	BLOGIC_UNCHECKED_TX = 0,
	BLOGIC_DATAIN_CHECKED = 1,
	BLOGIC_DATAOUT_CHECKED = 2,
	BLOGIC_NOTX = 3
};
/* …（同文件无关代码省略）… */
enum blogic_adapter_status {
	BLOGIC_CMD_CMPLT_NORMAL = 0x00,
	BLOGIC_LINK_CMD_CMPLT = 0x0A,
	BLOGIC_LINK_CMD_CMPLT_FLAG = 0x0B,
	BLOGIC_DATA_UNDERRUN = 0x0C,
	BLOGIC_SELECT_TIMEOUT = 0x11,
	BLOGIC_DATA_OVERRUN = 0x12,
	BLOGIC_NOEXPECT_BUSFREE = 0x13,
	BLOGIC_INVALID_BUSPHASE = 0x14,
	BLOGIC_INVALID_OUTBOX_CODE = 0x15,
	BLOGIC_INVALID_CMD_CODE = 0x16,
	BLOGIC_LINKCCB_BADLUN = 0x17,
	BLOGIC_BAD_CMD_PARAM = 0x1A,
	BLOGIC_AUTOREQSENSE_FAIL = 0x1B,
	BLOGIC_TAGQUEUE_REJECT = 0x1C,
	BLOGIC_BAD_MSG_RCVD = 0x1D,
	BLOGIC_HW_FAIL = 0x20,
	BLOGIC_NORESPONSE_TO_ATN = 0x21,
	BLOGIC_HW_RESET = 0x22,
	BLOGIC_RST_FROM_OTHERDEV = 0x23,
	BLOGIC_BAD_RECONNECT = 0x24,
	BLOGIC_HW_BDR = 0x25,
	BLOGIC_ABRT_QUEUE = 0x26,
	BLOGIC_ADAPTER_SW_ERROR = 0x27,
	BLOGIC_HW_TIMEOUT = 0x30,
	BLOGIC_PARITY_ERR = 0x34
} PACKED;
/* …（同文件无关代码省略）… */
enum blogic_tgt_status {
	BLOGIC_OP_GOOD = 0x00,
	BLOGIC_CHECKCONDITION = 0x02,
	BLOGIC_DEVBUSY = 0x08
} PACKED;
/* …（同文件无关代码省略）… */
enum blogic_queuetag {
	BLOGIC_SIMPLETAG = 0,
	BLOGIC_HEADTAG = 1,
	BLOGIC_ORDEREDTAG = 2,
	BLOGIC_RSVDTAG = 3
};
/* …（同文件无关代码省略）… */
#define BLOGIC_CDB_MAXLEN			12
/* …（同文件无关代码省略）… */
struct blogic_sg_seg {
	u32 segbytes;	/* Bytes 0-3 */
	u32 segdata;	/* Bytes 4-7 */
};
/* …（同文件无关代码省略）… */
enum blogic_ccb_status {
	BLOGIC_CCB_FREE = 0,
	BLOGIC_CCB_ACTIVE = 1,
	BLOGIC_CCB_COMPLETE = 2,
	BLOGIC_CCB_RESET = 3
} PACKED;
/* …（同文件无关代码省略）… */
struct blogic_ccb {
	/*
	   MultiMaster Firmware and FlashPoint SCCB Manager Common Portion.
	 */
	enum blogic_ccb_opcode opcode;			/* Byte 0 */
	unsigned char:3;				/* Byte 1 Bits 0-2 */
	enum blogic_datadir datadir:2;			/* Byte 1 Bits 3-4 */
	bool tag_enable:1;				/* Byte 1 Bit 5 */
	enum blogic_queuetag queuetag:2;		/* Byte 1 Bits 6-7 */
	unsigned char cdblen;				/* Byte 2 */
	unsigned char sense_datalen;			/* Byte 3 */
	u32 datalen;					/* Bytes 4-7 */
	u32 data;					/* Bytes 8-11 */
	unsigned char:8;				/* Byte 12 */
	unsigned char:8;				/* Byte 13 */
	enum blogic_adapter_status adapter_status;	/* Byte 14 */
	enum blogic_tgt_status tgt_status;		/* Byte 15 */
	unsigned char tgt_id;				/* Byte 16 */
	unsigned char lun:5;				/* Byte 17 Bits 0-4 */
	bool legacytag_enable:1;			/* Byte 17 Bit 5 */
	enum blogic_queuetag legacy_tag:2;		/* Byte 17 Bits 6-7 */
	unsigned char cdb[BLOGIC_CDB_MAXLEN];		/* Bytes 18-29 */
	unsigned char:8;				/* Byte 30 */
	unsigned char:8;				/* Byte 31 */
	u32 rsvd_int;					/* Bytes 32-35 */
	u32 sensedata;					/* Bytes 36-39 */
	/*
	   FlashPoint SCCB Manager Defined Portion.
	 */
	void (*callback) (struct blogic_ccb *);		/* Bytes 40-43 */
	u32 base_addr;					/* Bytes 44-47 */
	enum blogic_cmplt_code comp_code;		/* Byte 48 */
#ifdef CONFIG_SCSI_FLASHPOINT
	unsigned char:8;				/* Byte 49 */
	u16 os_flags;					/* Bytes 50-51 */
	unsigned char private[24];			/* Bytes 52-99 */
	void *rsvd1;
	void *rsvd2;
	unsigned char private2[16];
#endif
	/*
	   BusLogic Linux Driver Defined Portion.
	 */
	dma_addr_t allocgrp_head;
	unsigned int allocgrp_size;
	u32 dma_handle;
	enum blogic_ccb_status status;
	unsigned long serial;
	struct scsi_cmnd *command;
	struct blogic_adapter *adapter;
	struct blogic_ccb *next;
	struct blogic_ccb *next_all;
	struct blogic_sg_seg sglist[BLOGIC_SG_LIMIT];
};
/* …（同文件无关代码省略）… */
struct blogic_outbox {
	u32 ccb;			/* Bytes 0-3 */
	u32:24;				/* Bytes 4-6 */
	enum blogic_action action;	/* Byte 7 */
};
/* …（同文件无关代码省略）… */
struct blogic_inbox {
	u32 ccb;					/* Bytes 0-3 */
	enum blogic_adapter_status adapter_status;	/* Byte 4 */
	enum blogic_tgt_status tgt_status;		/* Byte 5 */
	unsigned char:8;				/* Byte 6 */
	enum blogic_cmplt_code comp_code;		/* Byte 7 */
};
/* …（同文件无关代码省略）… */
struct blogic_drvr_options {
	unsigned short tagq_ok;
	unsigned short tagq_ok_mask;
	unsigned short bus_settle_time;
	unsigned short stop_tgt_inquiry;
	unsigned char common_qdepth;
	unsigned char qdepth[BLOGIC_MAXDEV];
};
/* …（同文件无关代码省略）… */
struct blogic_tgt_flags {
	bool tgt_exists:1;
	bool tagq_ok:1;
	bool wide_ok:1;
	bool tagq_active:1;
	bool wide_active:1;
	bool cmd_good:1;
	bool tgt_info_in:1;
};
/* …（同文件无关代码省略）… */
#define BLOGIC_SZ_BUCKETS			10
/* …（同文件无关代码省略）… */
struct blogic_tgt_stats {
	unsigned int cmds_tried;
	unsigned int cmds_complete;
	unsigned int read_cmds;
	unsigned int write_cmds;
	struct blogic_byte_count bytesread;
	struct blogic_byte_count byteswritten;
	unsigned int read_sz_buckets[BLOGIC_SZ_BUCKETS];
	unsigned int write_sz_buckets[BLOGIC_SZ_BUCKETS];
	unsigned short aborts_request;
	unsigned short aborts_tried;
	unsigned short aborts_done;
	unsigned short bdr_request;
	unsigned short bdr_tried;
	unsigned short bdr_done;
	unsigned short adapter_reset_req;
	unsigned short adapter_reset_attempt;
	unsigned short adapter_reset_done;
};
/* …（同文件无关代码省略）… */
struct fpoint_info {
	u32 base_addr;				/* Bytes 0-3 */
	bool present;				/* Byte 4 */
	unsigned char irq_ch;			/* Byte 5 */
	unsigned char scsi_id;			/* Byte 6 */
	unsigned char scsi_lun;			/* Byte 7 */
	u16 fw_rev;				/* Bytes 8-9 */
	u16 sync_ok;				/* Bytes 10-11 */
	u16 fast_ok;				/* Bytes 12-13 */
	u16 ultra_ok;				/* Bytes 14-15 */
	u16 discon_ok;				/* Bytes 16-17 */
	u16 wide_ok;				/* Bytes 18-19 */
	bool parity:1;				/* Byte 20 Bit 0 */
	bool wide:1;				/* Byte 20 Bit 1 */
	bool softreset:1;			/* Byte 20 Bit 2 */
	bool ext_trans_enable:1;		/* Byte 20 Bit 3 */
	bool low_term:1;			/* Byte 20 Bit 4 */
	bool high_term:1;			/* Byte 20 Bit 5 */
	bool report_underrun:1;			/* Byte 20 Bit 6 */
	bool scam_enabled:1;			/* Byte 20 Bit 7 */
	bool scam_lev2:1;			/* Byte 21 Bit 0 */
	unsigned char:7;			/* Byte 21 Bits 1-7 */
	unsigned char family;			/* Byte 22 */
	unsigned char bus_type;			/* Byte 23 */
	unsigned char model[3];			/* Bytes 24-26 */
	unsigned char relative_cardnum;		/* Byte 27 */
	unsigned char rsvd[4];			/* Bytes 28-31 */
	u32 os_rsvd;				/* Bytes 32-35 */
	unsigned char translation_info[4];	/* Bytes 36-39 */
	u32 rsvd2[5];				/* Bytes 40-59 */
	u32 sec_range;				/* Bytes 60-63 */
};
/* …（同文件无关代码省略）… */
struct blogic_adapter {
	struct Scsi_Host *scsi_host;
	struct pci_dev *pci_device;
	enum blogic_adapter_type adapter_type;
	enum blogic_adapter_bus_type adapter_bus_type;
	unsigned long io_addr;
	unsigned long pci_addr;
	unsigned short addr_count;
	unsigned char host_no;
	unsigned char model[9];
	unsigned char fw_ver[6];
	unsigned char full_model[18];
	unsigned char bus;
	unsigned char dev;
	unsigned char irq_ch;
	unsigned char scsi_id;
	bool irq_acquired:1;
	bool ext_trans_enable:1;
	bool parity:1;
	bool reset_enabled:1;
	bool level_int:1;
	bool wide:1;
	bool differential:1;
	bool scam:1;
	bool ultra:1;
	bool ext_lun:1;
	bool terminfo_valid:1;
	bool low_term:1;
	bool high_term:1;
	bool strict_rr:1;
	bool scam_enabled:1;
	bool scam_lev2:1;
	bool adapter_initd:1;
	bool adapter_extreset:1;
	bool adapter_intern_err:1;
	bool processing_ccbs;
	volatile bool adapter_cmd_complete;
	unsigned short adapter_sglimit;
	unsigned short drvr_sglimit;
	unsigned short maxdev;
	unsigned short maxlun;
	unsigned short mbox_count;
	unsigned short initccbs;
	unsigned short inc_ccbs;
	unsigned short alloc_ccbs;
	unsigned short drvr_qdepth;
	unsigned short adapter_qdepth;
	unsigned short untag_qdepth;
	unsigned short common_qdepth;
	unsigned short bus_settle_time;
	unsigned short sync_ok;
	unsigned short fast_ok;
	unsigned short ultra_ok;
	unsigned short wide_ok;
	unsigned short discon_ok;
	unsigned short tagq_ok;
	unsigned short ext_resets;
	unsigned short adapter_intern_errors;
	unsigned short tgt_count;
	unsigned short msgbuflen;
	u32 bios_addr;
	struct blogic_drvr_options *drvr_opts;
	struct fpoint_info fpinfo;
	void *cardhandle;
	struct list_head host_list;
	struct blogic_ccb *all_ccbs;
	struct blogic_ccb *free_ccbs;
	struct blogic_ccb *firstccb;
	struct blogic_ccb *lastccb;
	struct blogic_ccb *bdr_pend[BLOGIC_MAXDEV];
	struct blogic_tgt_flags tgt_flags[BLOGIC_MAXDEV];
	unsigned char qdepth[BLOGIC_MAXDEV];
	unsigned char sync_period[BLOGIC_MAXDEV];
	unsigned char sync_offset[BLOGIC_MAXDEV];
	unsigned char active_cmds[BLOGIC_MAXDEV];
	unsigned int cmds_since_rst[BLOGIC_MAXDEV];
	unsigned long last_seqpoint[BLOGIC_MAXDEV];
	unsigned long last_resettried[BLOGIC_MAXDEV];
	unsigned long last_resetdone[BLOGIC_MAXDEV];
	struct blogic_outbox *first_outbox;
	struct blogic_outbox *last_outbox;
	struct blogic_outbox *next_outbox;
	struct blogic_inbox *first_inbox;
	struct blogic_inbox *last_inbox;
	struct blogic_inbox *next_inbox;
	struct blogic_tgt_stats tgt_stats[BLOGIC_MAXDEV];
	unsigned char *mbox_space;
	dma_addr_t mbox_space_handle;
	unsigned int mbox_sz;
	unsigned long ccb_offset;
	char msgbuf[BLOGIC_MSGBUF_SIZE];
};
/* …（同文件无关代码省略）… */
				 struct queue_limits *lim);
static void blogic_qcompleted_ccb(struct blogic_ccb *);
static irqreturn_t blogic_inthandler(int, void *);
static int blogic_resetadapter(struct blogic_adapter *, bool hard_reset);
static void blogic_msg(enum blogic_msglevel, char *, struct blogic_adapter *, ...);
static int __init blogic_setup(char *);
