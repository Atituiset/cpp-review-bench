// AUTO-DRAFT from torvalds/linux PR #87aecd24f58e15256870befed777bbffc5d4a9d5
		return ERR_PTR(-ENOMEM);
	}
  // <<< BUG ANCHOR
	cmd->version = metadata_version;
	refcount_set(&cmd->ref_count, 1);
	init_rwsem(&cmd->root_lock);
/* …（同文件无关代码省略）… */
					   from_cblock(cmd->cache_blocks),
					   &cmd->dirty_cursor);
		if (r) {
			dm_array_cursor_end(&cmd->hint_cursor);
			dm_array_cursor_end(&cmd->mapping_cursor);
			return r;
		}
