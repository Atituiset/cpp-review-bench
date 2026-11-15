// AUTO-DRAFT from torvalds/linux PR #68271fbe7d14b6397e90df730c3967d3a481902b
				ntfs_error(vol->sb, "Failed to merge runlists");
				up_write(&ni->runlist.lock);
				mutex_unlock(&mrec_ni->mrec_lock);
				kvfree(rlc);  // <<< BUG ANCHOR
				return PTR_ERR(rl);
			}
