// AUTO-DRAFT from postgres/postgres PR #b5545d3f5f924289051eb8221c88e72563771928
			BlockRefTableSetLimitBlock(brtab, &xlrec->rlocator,
									   MAIN_FORKNUM, xlrec->blkno);
		if ((xlrec->flags & SMGR_TRUNCATE_VM) != 0)
			BlockRefTableSetLimitBlock(brtab, &xlrec->rlocator,
									   VISIBILITYMAP_FORKNUM,  // <<< BUG ANCHOR
									   visibilitymap_truncation_length(xlrec->blkno));
	}
}
