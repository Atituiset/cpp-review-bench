// AUTO-DRAFT from torvalds/linux PR #0f2732620d6ec49d3e6c06831575eaec6bc99d42
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#define SALT_SIZE			16
/* …（同文件无关代码省略）… */
#define SB_SECTORS			8
/* …（同文件无关代码省略）… */
struct superblock {
	__u8 magic[8];
	__u8 version;
	__u8 log2_interleave_sectors;
	__le16 integrity_tag_size;
	__le32 journal_sections;
	__le64 provided_data_sectors;	/* userspace uses this value */
	__le32 flags;
	__u8 log2_sectors_per_block;
	__u8 log2_blocks_per_bitmap_bit;
	__u8 pad[2];
	__le64 recalc_sector;
	__u8 pad2[8];
	__u8 salt[SALT_SIZE];
};
/* …（同文件无关代码省略）… */
#define SB_FLAG_FIXED_HMAC		0x10
/* …（同文件无关代码省略）… */
typedef __le64 commit_id_t;
/* …（同文件无关代码省略）… */
#define N_COMMIT_IDS			4
/* …（同文件无关代码省略）… */
struct journal_node {
	struct rb_node node;
	sector_t sector;
};
/* …（同文件无关代码省略）… */
struct alg_spec {
	char *alg_string;
	char *key_string;
	__u8 *key;
	unsigned int key_size;
};
/* …（同文件无关代码省略）… */
struct dm_integrity_c {
	struct dm_dev *dev;
	struct dm_dev *meta_dev;
	unsigned int tag_size;
	__s8 log2_tag_size;
	unsigned int tuple_size;
	sector_t start;
	mempool_t journal_io_mempool;
	struct dm_io_client *io;
	struct dm_bufio_client *bufio;
	struct workqueue_struct *metadata_wq;
	struct superblock *sb;
	unsigned int journal_pages;
	unsigned int n_bitmap_blocks;

	struct page_list *journal;
	struct page_list *journal_io;
	struct page_list *journal_xor;
	struct page_list *recalc_bitmap;
	struct page_list *may_write_bitmap;
	struct bitmap_block_status *bbs;
	unsigned int bitmap_flush_interval;
	int synchronous_mode;
	struct bio_list synchronous_bios;
	struct delayed_work bitmap_flush_work;

	struct crypto_skcipher *journal_crypt;
	struct scatterlist **journal_scatterlist;
	struct scatterlist **journal_io_scatterlist;
	struct skcipher_request **sk_requests;

	struct crypto_shash *journal_mac;

	struct journal_node *journal_tree;
	struct rb_root journal_tree_root;

	sector_t provided_data_sectors;

	unsigned short journal_entry_size;
	unsigned char journal_entries_per_sector;
	unsigned char journal_section_entries;
	unsigned short journal_section_sectors;
	unsigned int journal_sections;
	unsigned int journal_entries;
	sector_t data_device_sectors;
	sector_t meta_device_sectors;
	unsigned int initial_sectors;
	unsigned int metadata_run;
	__s8 log2_metadata_run;
	__u8 log2_buffer_sectors;
	__u8 sectors_per_block;
	__u8 log2_blocks_per_bitmap_bit;

	unsigned char mode;
	bool internal_hash;

	int failed;

	struct crypto_shash *internal_shash;
	struct crypto_ahash *internal_ahash;
	unsigned int internal_hash_digestsize;

	struct dm_target *ti;

	/* these variables are locked with endio_wait.lock */
	struct rb_root in_progress;
	struct list_head wait_list;
	wait_queue_head_t endio_wait;
	struct workqueue_struct *wait_wq;
	struct workqueue_struct *offload_wq;

	unsigned char commit_seq;
	commit_id_t commit_ids[N_COMMIT_IDS];

	unsigned int committed_section;
	unsigned int n_committed_sections;

	unsigned int uncommitted_section;
	unsigned int n_uncommitted_sections;

	unsigned int free_section;
	unsigned char free_section_entry;
	unsigned int free_sectors;

	unsigned int free_sectors_threshold;

	struct workqueue_struct *commit_wq;
	struct work_struct commit_work;

	struct workqueue_struct *writer_wq;
	struct work_struct writer_work;

	struct workqueue_struct *recalc_wq;
	struct work_struct recalc_work;

	struct bio_list flush_bio_list;

	unsigned long autocommit_jiffies;
	struct timer_list autocommit_timer;
	unsigned int autocommit_msec;

	wait_queue_head_t copy_to_journal_wait;

	struct completion crypto_backoff;

	bool wrote_to_journal;
	bool journal_uptodate;
	bool just_formatted;
	bool recalculate_flag;
	bool reset_recalculate_flag;
	bool discard;
	bool discard_keyed;
	bool fix_padding;
	bool fix_hmac;
	bool legacy_recalculate;

	mempool_t ahash_req_pool;
	struct ahash_request *journal_ahash_req;

	struct alg_spec internal_hash_alg;
	struct alg_spec journal_crypt_alg;
	struct alg_spec journal_mac_alg;

	atomic64_t number_of_mismatches;

	mempool_t recheck_pool;
	struct bio_set recheck_bios;
	struct bio_set recalc_bios;

	struct notifier_block reboot_notifier;
};
/* …（同文件无关代码省略）… */
struct bitmap_block_status {
	struct work_struct work;
	struct dm_integrity_c *ic;
	unsigned int idx;
	unsigned long *bitmap;
	struct bio_list bio_queue;
	spinlock_t bio_queue_lock;

};
/* …（同文件无关代码省略）… */
static void dm_integrity_io_error(struct dm_integrity_c *ic, const char *msg, int err)
{
	if (err == -EILSEQ)
		atomic64_inc(&ic->number_of_mismatches);
	if (!cmpxchg(&ic->failed, 0, err))
		DMERR("Error on %s: %d", msg, err);
}
/* …（同文件无关代码省略）… */
static void integrity_sector_checksum_shash(struct dm_integrity_c *ic, sector_t sector,
					    const char *data, unsigned offset,
					    unsigned int len, char *result)
{
	__le64 sector_le = cpu_to_le64(sector);
	SHASH_DESC_ON_STACK(req, ic->internal_shash);
	int r;
	unsigned int digest_size;

	req->tfm = ic->internal_shash;

	r = crypto_shash_init(req);
	if (unlikely(r < 0)) {
		dm_integrity_io_error(ic, "crypto_shash_init", r);
		goto failed;
	}

	if (ic->sb->flags & cpu_to_le32(SB_FLAG_FIXED_HMAC)) {
		r = crypto_shash_update(req, (__u8 *)&ic->sb->salt, SALT_SIZE);
		if (unlikely(r < 0)) {
			dm_integrity_io_error(ic, "crypto_shash_update", r);
			goto failed;
		}
	}

	r = crypto_shash_update(req, (const __u8 *)&sector_le, sizeof(sector_le));
	if (unlikely(r < 0)) {
		dm_integrity_io_error(ic, "crypto_shash_update", r);
		goto failed;
	}

	if (likely(len)) {
		r = crypto_shash_update(req, data + offset, len);
		if (unlikely(r < 0)) {
			dm_integrity_io_error(ic, "crypto_shash_update", r);
			goto failed;
		}
	}

	r = crypto_shash_final(req, result);
	if (unlikely(r < 0)) {
		dm_integrity_io_error(ic, "crypto_shash_final", r);
		goto failed;
	}

	digest_size = ic->internal_hash_digestsize;
	if (unlikely(digest_size < ic->tag_size))
		memset(result + digest_size, 0, ic->tag_size - digest_size);

	return;

failed:
	/* this shouldn't happen anyway, the hash functions have no reason to fail */
	get_random_bytes(result, ic->tag_size);
}
/* …（同文件无关代码省略）… */
static void integrity_sector_checksum_ahash(struct dm_integrity_c *ic, struct ahash_request **ahash_req,
					    sector_t sector, struct page *page, unsigned offset,
					    unsigned int len, char *result)
{
	__le64 sector_le = cpu_to_le64(sector);
	struct ahash_request *req;
	DECLARE_CRYPTO_WAIT(wait);
	struct scatterlist sg[3], *s = sg;
	int r;
	unsigned int digest_size;
	unsigned int nbytes = 0;
	unsigned int nents = 1 + (len ? 1 : 0);

	might_sleep();

	req = *ahash_req;
	if (unlikely(!req)) {
		req = mempool_alloc(&ic->ahash_req_pool, GFP_NOIO);
		*ahash_req = req;
	}

	ahash_request_set_tfm(req, ic->internal_ahash);
	ahash_request_set_callback(req, CRYPTO_TFM_REQ_MAY_SLEEP, crypto_req_done, &wait);

	if (ic->sb->flags & cpu_to_le32(SB_FLAG_FIXED_HMAC)) {
		sg_init_table(sg, nents + 1);
		sg_set_buf(s, (const __u8 *)&ic->sb->salt, SALT_SIZE);
		nbytes += SALT_SIZE;
		s++;
	} else {
		sg_init_table(sg, nents);
	}

	if (likely(!is_vmalloc_addr(&sector_le))) {
		sg_set_buf(s, &sector_le, sizeof(sector_le));
	} else {
		struct page *sec_page = vmalloc_to_page(&sector_le);
		unsigned int sec_off = offset_in_page(&sector_le);
		sg_set_page(s, sec_page, sizeof(sector_le), sec_off);
	}
	nbytes += sizeof(sector_le);
	s++;

	if (likely(len)) {
		sg_set_page(s, page, len, offset);
		nbytes += len;
	}

	ahash_request_set_crypt(req, sg, result, nbytes);

	r = crypto_wait_req(crypto_ahash_digest(req), &wait);
	if (unlikely(r)) {
		dm_integrity_io_error(ic, "crypto_ahash_digest", r);
		goto failed;
	}

	digest_size = ic->internal_hash_digestsize;
	if (unlikely(digest_size < ic->tag_size))
		memset(result + digest_size, 0, ic->tag_size - digest_size);

	return;

failed:
	/* this shouldn't happen anyway, the hash functions have no reason to fail */
	get_random_bytes(result, ic->tag_size);
}
/* …（同文件无关代码省略）… */
static void integrity_sector_checksum(struct dm_integrity_c *ic, struct ahash_request **ahash_req,
				      sector_t sector, const char *data, unsigned offset, char *result)
{
	unsigned int len = ic->sectors_per_block << SECTOR_SHIFT;

	if (likely(ic->internal_shash != NULL))
		integrity_sector_checksum_shash(ic, sector, data, offset, len, result);
	else
		integrity_sector_checksum_ahash(ic, ahash_req, sector, (struct page *)data,
						 offset, len, result);
}
/* …（同文件无关代码省略）… */
static void *integrity_kmap(struct dm_integrity_c *ic, struct page *p)
{
	if (likely(ic->internal_shash != NULL))
		return kmap_local_page(p);
	else
		return p;
}
/* …（同文件无关代码省略）… */
	outgoing_data = dio->integrity_payload + PAGE_SIZE;

	while (dio->bio_details.bi_iter.bi_size) {
		char digest[HASH_MAX_DIGESTSIZE];
		int r;
		struct bio_integrity_payload *bip;
		struct bio_vec bv;
/* …（同文件无关代码省略）… */
	unsigned pos = 0;

	while (dio->bio_details.bi_iter.bi_size) {
		char digest[HASH_MAX_DIGESTSIZE];
		struct bio_vec bv = bio_iter_iovec(bio, dio->bio_details.bi_iter);
		char *mem = integrity_kmap(ic, bv.bv_page);
		integrity_sector_checksum(ic, &dio->ahash_req, dio->bio_details.bi_iter.bi_sector, mem, bv.bv_offset, digest);
/* …（同文件无关代码省略）… */
	ic->journal_uptodate = true;
}

static void dm_integrity_resume(struct dm_target *ti)
{
	struct dm_integrity_c *ic = ti->private;
/* …（同文件无关代码省略）… */
	if (r)
		dm_integrity_io_error(ic, "reading superblock", r);

	if (ic->mode == 'R')
		goto skip_writes;

/* …（同文件无关代码省略）… */
		ic->just_formatted = true;
	}

	if (!ic->meta_dev && ic->mode != 'I') {
		r = dm_set_target_max_io_len(ti, 1U << ic->sb->log2_interleave_sectors);
		if (r)
/* …（同文件无关代码省略）… */
	kvfree(ic->journal_tree);
	if (ic->sb)
		free_pages_exact(ic->sb, SB_SECTORS << SECTOR_SHIFT);

	if (ic->internal_shash)
		crypto_free_shash(ic->internal_shash);
