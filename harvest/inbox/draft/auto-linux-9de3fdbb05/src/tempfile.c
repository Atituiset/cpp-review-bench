// AUTO-DRAFT from torvalds/linux PR #f802b6558eb81bd5888d761a0cdc8afe5fd8c941
	struct xfs_buf		*bp;
	xfs_fileoff_t		flush_mask;
	xfs_fileoff_t		end = off + len;
	loff_t			pos = XFS_FSB_TO_B(mp, off);  // <<< BUG ANCHOR
	int			error = 0;

	ASSERT(S_ISREG(VFS_I(sc->tempip)->i_mode));

	/* Flush buffers to disk every 512K */
	flush_mask = XFS_B_TO_FSBT(mp, (1U << 19)) - 1;

	for (; off < end; off++, pos += mp->m_sb.sb_blocksize) {
		struct xfs_bmbt_irec	map;
		int			nmaps = 1;
