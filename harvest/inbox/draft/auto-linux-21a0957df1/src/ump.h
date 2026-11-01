// AUTO-DRAFT from torvalds/linux PR #e9954f3fbddd25e1a01b7e5a0b72fb8b564204e0
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

struct snd_ump_group {
	int group;			/* group index (0-based) */
	unsigned int dir_bits;		/* directions */
	bool active;			/* activeness */
	bool valid;			/* valid group (referred by blocks) */
	bool is_midi1;			/* belongs to a MIDI1 FB */
	char name[64];			/* group name */
};
/* …（同文件无关代码省略）… */
struct snd_ump_endpoint {
	struct snd_rawmidi core;	/* raw UMP access */

	struct snd_ump_endpoint_info info;

	const struct snd_ump_ops *ops;	/* UMP ops set by the driver */
	struct snd_rawmidi_substream *substreams[2];	/* opened substreams */

	void *private_data;
	void (*private_free)(struct snd_ump_endpoint *ump);

	/* UMP Stream message processing */
	u32 stream_wait_for;	/* expected stream message status */
	bool stream_finished;	/* set when message has been processed */
	bool parsed;		/* UMP / FB parse finished? */
	bool no_process_stream;	/* suppress UMP stream messages handling */
	wait_queue_head_t stream_wait;
	struct snd_rawmidi_file stream_rfile;

	struct list_head block_list;	/* list of snd_ump_block objects */

	/* intermediate buffer for UMP input */
	u32 input_buf[4];
	int input_buf_head;
	int input_pending;

	struct mutex open_mutex;

	struct snd_ump_group groups[SNDRV_UMP_MAX_GROUPS]; /* table of groups */

#if IS_ENABLED(CONFIG_SND_UMP_LEGACY_RAWMIDI)
	spinlock_t legacy_locks[2];
	struct snd_rawmidi *legacy_rmidi;
	struct snd_rawmidi_substream *legacy_substreams[2][SNDRV_UMP_MAX_GROUPS];
	unsigned char legacy_mapping[SNDRV_UMP_MAX_GROUPS];

	/* for legacy output; need to open the actual substream unlike input */
	int legacy_out_opens;
	struct snd_rawmidi_file legacy_out_rfile;
	struct ump_cvt_to_ump *out_cvts;
#endif

#if IS_ENABLED(CONFIG_SND_SEQUENCER)
	struct snd_seq_device *seq_dev;
	const struct snd_seq_ump_ops *seq_ops;
	void *seq_client;
#endif
};
/* …（同文件无关代码省略）… */
struct snd_ump_ops {
	int (*open)(struct snd_ump_endpoint *ump, int dir);
	void (*close)(struct snd_ump_endpoint *ump, int dir);
	void (*trigger)(struct snd_ump_endpoint *ump, int dir, int up);
	void (*drain)(struct snd_ump_endpoint *ump, int dir);
};
/* …（同文件无关代码省略）… */
struct snd_seq_ump_ops {
	void (*input_receive)(struct snd_ump_endpoint *ump,
			      const u32 *data, int words);
	int (*notify_ep_change)(struct snd_ump_endpoint *ump);
	int (*notify_fb_change)(struct snd_ump_endpoint *ump,
				struct snd_ump_block *fb);
	int (*switch_protocol)(struct snd_ump_endpoint *ump);
};
/* …（同文件无关代码省略）… */
struct snd_ump_block {
	struct snd_ump_block_info info;
	struct snd_ump_endpoint *ump;

	void *private_data;
	void (*private_free)(struct snd_ump_block *blk);

	struct list_head list;
};
/* …（同文件无关代码省略）… */
int snd_ump_switch_protocol(struct snd_ump_endpoint *ump, unsigned int protocol);
void snd_ump_update_group_attrs(struct snd_ump_endpoint *ump);

/*
 * Some definitions for UMP
 */
