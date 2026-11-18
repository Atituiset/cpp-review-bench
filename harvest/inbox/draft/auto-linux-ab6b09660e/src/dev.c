// AUTO-DRAFT from torvalds/linux PR #63fdb511c27afcebce797e3db580b3534ddfa639
static int unlock_request(struct fuse_req *req)
{
	int err = 0;
	if (req) {
		spin_lock(&req->waitq.lock);
		if (test_bit(FR_ABORTED, &req->flags))
			err = -ENOENT;
		else
			clear_bit(FR_LOCKED, &req->flags);
		spin_unlock(&req->waitq.lock);
	}
	return err;
}
/* …（同文件无关代码省略）… */
	if (cs->nr_segs >= cs->pipe->max_usage)
		return -EIO;
  // <<< BUG ANCHOR
	folio_get(folio);
	err = unlock_request(cs->req);
	if (err) {
		folio_put(folio);
