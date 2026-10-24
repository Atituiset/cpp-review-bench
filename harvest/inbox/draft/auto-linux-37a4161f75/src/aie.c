// AUTO-DRAFT from torvalds/linux PR #5b0764a53235d3c77e1ed674e836e1b3fe1a4b1f
static void amdxdna_update_vbnv(struct amdxdna_dev *xdna,
				const struct amdxdna_rev_vbnv *tbl,
				u32 rev)
{
	int i;

	for (i = 0; tbl[i].vbnv; i++) {
		if (tbl[i].revision == rev) {
			xdna->vbnv = tbl[i].vbnv;
			break;
		}
	}
}
/* …（同文件无关代码省略）… */
	amdxdna_update_vbnv(xdna, info->rev_vbnv_tbl, rev);
}

int amdxdna_get_metadata(struct aie_device *aie,
			 struct amdxdna_client *client,
			 struct amdxdna_drm_get_info *args)
{
	int ret = 0;
	u32 buf_sz;

	buf_sz = min(args->buffer_size, sizeof(aie->metadata));
	if (copy_to_user(u64_to_user_ptr(args->buffer), &aie->metadata, buf_sz))
		ret = -EFAULT;

	return ret;
}
