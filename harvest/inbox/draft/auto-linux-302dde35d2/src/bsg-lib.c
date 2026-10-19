// AUTO-DRAFT from torvalds/linux PR #f909e534f5e34feb2ea2701d54df63e21aa506fc
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <string.h>
  // <<< BUG ANCHOR
struct bsg_set {
	struct blk_mq_tag_set	tag_set;
	struct bsg_device	*bd;
	bsg_job_fn		*job_fn;
	bsg_timeout_fn		*timeout_fn;
};

static int bsg_transport_sg_io_fn(struct request_queue *q, struct sg_io_v4 *hdr,
		bool open_for_write, unsigned int timeout)
{
	struct bsg_job *job;
	struct request *rq;
	struct bio *bio;
/* …（同文件无关代码省略）… */
	reply = job->reply;
	memset(job, 0, sizeof(*job));
	job->reply = reply;
	job->reply_len = SCSI_SENSE_BUFFERSIZE;
	job->dd_data = job + 1;

	job->request_len = hdr->request_len;
/* …（同文件无关代码省略）… */
static int bsg_init_rq(struct blk_mq_tag_set *set, struct request *req,
		       unsigned int hctx_idx, int numa_node)
{
	struct bsg_job *job = blk_mq_rq_to_pdu(req);

	job->reply = kzalloc(SCSI_SENSE_BUFFERSIZE, GFP_KERNEL);
	if (!job->reply)
		return -ENOMEM;
	return 0;
/* …（同文件无关代码省略）… */
 * @job_fn: bsg job handler
 * @timeout: timeout handler function pointer
 * @dd_job_size: size of LLD data needed for each job
 */
struct request_queue *bsg_setup_queue(struct device *dev, const char *name,
		struct queue_limits *lim, bsg_job_fn *job_fn,
		bsg_timeout_fn *timeout, int dd_job_size)
{
	struct bsg_set *bset;
	struct blk_mq_tag_set *set;
/* …（同文件无关代码省略）… */

	bset->job_fn = job_fn;
	bset->timeout_fn = timeout;

	set = &bset->tag_set;
	set->ops = &bsg_mq_ops;
