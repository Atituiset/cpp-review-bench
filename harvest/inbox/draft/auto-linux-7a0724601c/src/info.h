// AUTO-DRAFT from torvalds/linux PR #e9954f3fbddd25e1a01b7e5a0b72fb8b564204e0
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>

struct snd_info_buffer {
	char *buffer;		/* pointer to begin of buffer */
	unsigned int curr;	/* current position in buffer */
	unsigned int size;	/* current size */
	unsigned int len;	/* total length of buffer */
	int stop;		/* stop flag */
	int error;		/* error code */
};
/* …（同文件无关代码省略）… */
struct snd_info_entry_text {
	void (*read)(struct snd_info_entry *entry,
		     struct snd_info_buffer *buffer);
	void (*write)(struct snd_info_entry *entry,
		      struct snd_info_buffer *buffer);
};
/* …（同文件无关代码省略）… */
struct snd_info_entry_ops {
	int (*open)(struct snd_info_entry *entry,
		    unsigned short mode, void **file_private_data);
	int (*release)(struct snd_info_entry *entry,
		       unsigned short mode, void *file_private_data);
	ssize_t (*read)(struct snd_info_entry *entry, void *file_private_data,
			struct file *file, char __user *buf,
			size_t count, loff_t pos);
	ssize_t (*write)(struct snd_info_entry *entry, void *file_private_data,
			 struct file *file, const char __user *buf,
			 size_t count, loff_t pos);
	loff_t (*llseek)(struct snd_info_entry *entry,
			 void *file_private_data, struct file *file,
			 loff_t offset, int orig);
	__poll_t (*poll)(struct snd_info_entry *entry,
			     void *file_private_data, struct file *file,
			     poll_table *wait);
	int (*ioctl)(struct snd_info_entry *entry, void *file_private_data,
		     struct file *file, unsigned int cmd, unsigned long arg);
	int (*mmap)(struct snd_info_entry *entry, void *file_private_data,
		    struct inode *inode, struct file *file,
		    struct vm_area_struct *vma);
};
/* …（同文件无关代码省略）… */
struct snd_info_entry {
	const char *name;
	umode_t mode;
	long size;
	unsigned short content;
	union {
		struct snd_info_entry_text text;
		const struct snd_info_entry_ops *ops;
	} c;
	struct snd_info_entry *parent;
	struct module *module;
	void *private_data;
	void (*private_free)(struct snd_info_entry *entry);
	struct proc_dir_entry *p;
	struct mutex access;
	struct list_head children;
	struct list_head list;
};
/* …（同文件无关代码省略）… */
struct snd_info_entry *snd_info_create_card_entry(struct snd_card *card,
					     const char *name,
					     struct snd_info_entry *parent);
void snd_info_free_entry(struct snd_info_entry *entry);

int snd_info_card_create(struct snd_card *card);
/* …（同文件无关代码省略）… */
static inline char *snd_info_get_str(char *dest, char *src, int len) { return NULL; }
static inline struct snd_info_entry *snd_info_create_module_entry(struct module *module, const char *name, struct snd_info_entry *parent) { return NULL; }
static inline struct snd_info_entry *snd_info_create_card_entry(struct snd_card *card, const char *name, struct snd_info_entry *parent) { return NULL; }
static inline void snd_info_free_entry(struct snd_info_entry *entry) { ; }

static inline int snd_info_card_create(struct snd_card *card) { return 0; }
