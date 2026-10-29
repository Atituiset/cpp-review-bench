// AUTO-DRAFT from torvalds/linux PR #c339f4010dea8b0d754f84ace70cd1b089878ef8

#define HSMP_MAX_MSG_LEN 8

/*
 * HSMP Messages supported
 */
/* …（同文件无关代码省略）… */
	HSMP_MSG_ID_MAX,
};

struct hsmp_message {
	__u32	msg_id;			/* Message ID */
	__u16	num_args;		/* Number of input argument words in message */
/* …（同文件无关代码省略）… */
	HSMP_PROTO_VER7
};

struct hsmp_msg_desc {
	int num_args;
	int response_sz;
/* …（同文件无关代码省略）… */
#define HSMP_BASE_IOCTL_NR	0xF8
/* …（同文件无关代码省略）… */
#define HSMP_IOCTL_GET_TELEMETRY_DATA \
	_IOW(HSMP_BASE_IOCTL_NR, 1, struct hsmp_telemetry_data)

#endif /*_ASM_X86_AMD_HSMP_H_*/
