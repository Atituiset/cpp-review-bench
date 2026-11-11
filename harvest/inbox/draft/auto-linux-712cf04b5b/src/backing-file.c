// AUTO-DRAFT from torvalds/linux PR #5f5be6584cf5af0d70ee2fd40aa6264da18617ec
  // <<< BUG ANCHOR
static struct kmem_cache *backing_aio_cachep;

#define BACKING_IOCB_MASK \
	(IOCB_NOWAIT | IOCB_HIPRI | IOCB_DSYNC | IOCB_SYNC | IOCB_APPEND)

static rwf_t iocb_to_rw_flags(int flags)
{
