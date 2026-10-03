// AUTO-DRAFT from torvalds/linux PR #5d144c294ae21c1bfb275ba06ec73c2d1ab7cf92
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

struct snd_refcount {
	atomic_t count;
	wait_queue_head_t waiter;
};
/* …（同文件无关代码省略）… */
struct snd_card {
	int number;			/* number of soundcard (index to
								snd_cards) */

	char id[16];			/* id string of this card */
	char driver[16];		/* driver name */
	char shortname[32];		/* short name of this soundcard */
	char longname[80];		/* name of this soundcard */
	char irq_descr[32];		/* Interrupt description */
	char mixername[80];		/* mixer name */
	char *components;		/* card components, space-delimited */
	unsigned int components_alloc_size;	/* current allocation size of components */
	struct module *module;		/* top-level module */

	void *private_data;		/* private data for soundcard */
	void (*private_free) (struct snd_card *card); /* callback for freeing of
								private data */
	struct list_head devices;	/* devices */

	struct device *ctl_dev;		/* control device */
	unsigned int last_numid;	/* last used numeric ID */
	struct rw_semaphore controls_rwsem;	/* controls lock (list and values) */
	rwlock_t controls_rwlock;	/* lock for lookup and ctl_files list */
	int controls_count;		/* count of all controls */
	size_t user_ctl_alloc_size;	// current memory allocation by user controls.
	struct list_head controls;	/* all controls for this card */
	struct list_head ctl_files;	/* active control files */
#ifdef CONFIG_SND_CTL_FAST_LOOKUP
	struct xarray ctl_numids;	/* hash table for numids */
	struct xarray ctl_hash;		/* hash table for ctl id matching */
	bool ctl_hash_collision;	/* ctl_hash collision seen? */
#endif

	struct snd_info_entry *proc_root;	/* root for soundcard specific files */
	struct proc_dir_entry *proc_root_link;	/* number link to real id */

	struct list_head files_list;	/* all files associated to this card */
	struct snd_shutdown_f_ops *s_f_ops; /* file operations in the shutdown
								state */
	spinlock_t files_lock;		/* lock the files for this card */
	int shutdown;			/* this card is going down */
	struct completion *release_completion;
	struct device *dev;		/* device assigned to this card */
	struct device card_dev;		/* cardX object for sysfs */
	const struct attribute_group *dev_groups[4]; /* assigned sysfs attr */
	bool registered;		/* card_dev is registered? */
	bool managed;			/* managed via devres */
	bool releasing;			/* during card free process */
	int sync_irq;			/* assigned irq, used for PCM sync */
	wait_queue_head_t remove_sleep;

	size_t total_pcm_alloc_bytes;	/* total amount of allocated buffers */
	struct mutex memory_mutex;	/* protection for the above */
#ifdef CONFIG_SND_DEBUG
	struct dentry *debugfs_root;    /* debugfs root for card */
#endif
#ifdef CONFIG_SND_CTL_DEBUG
	struct snd_ctl_elem_value *value_buf; /* buffer for kctl->put() verification */
#endif

#ifdef CONFIG_PM
	unsigned int power_state;	/* power state */
	wait_queue_head_t power_sleep;
	struct snd_refcount power_ref;
#endif

#if IS_ENABLED(CONFIG_SND_MIXER_OSS)
	struct snd_mixer_oss *mixer_oss;
	int mixer_oss_change_count;
#endif

	unsigned char private_data_area[] __aligned(__alignof__(unsigned long long));
};
/* …（同文件无关代码省略）… */
static inline void snd_card_unref(struct snd_card *card)
{
	put_device(&card->card_dev);
}

DEFINE_FREE(snd_card_unref, struct snd_card *, if (_T) snd_card_unref(_T))

#define snd_card_set_dev(card, devptr) ((card)->dev = (devptr))

/* device.c */
