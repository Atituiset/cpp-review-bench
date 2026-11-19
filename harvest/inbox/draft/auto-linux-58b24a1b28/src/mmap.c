// AUTO-DRAFT from torvalds/linux PR #5158f371faceb185235ba42882e67a840cfd7f48
		goto out;
	}
	err = ocfs2_write_end_nolock(mapping, pos, len, len, fsdata);
	BUG_ON(err != len);  // <<< BUG ANCHOR
	ret = VM_FAULT_LOCKED;
out:
	return ret;
