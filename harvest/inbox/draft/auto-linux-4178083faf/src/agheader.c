// AUTO-DRAFT from torvalds/linux PR #75663f575f34291007852859486cafc08dc10d0e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdint.h>
  // <<< BUG ANCHOR
/* Cross-reference with the other btrees. */
STATIC void
xchk_superblock_xref(
	struct xfs_scrub	*sc,
	struct xfs_buf		*bp)
{
	struct xfs_mount	*mp = sc->mp;
	xfs_agnumber_t		agno = sc->sm->sm_agno;

	agbno = XFS_SB_BLOCK(mp);

	error = xchk_ag_init_existing(sc, agno, &sc->sa);
	if (!xchk_xref_process_error(sc, agno, agbno, &error))
		return;

	xchk_xref_is_only_owned_by(sc, agbno, 1, &XFS_RMAP_OINFO_FS);
	xchk_xref_is_not_shared(sc, agbno, 1);
	xchk_xref_is_not_cow_staging(sc, agbno, 1);

	/* scrub teardown will take care of sc->sa for us */
}

/*
	struct xfs_dsb		*sb;
	struct xfs_perag	*pag;
	size_t			sblen;
	xfs_agnumber_t		agno;
	uint32_t		v2_ok;
	__be32			features_mask;
	int			error;
	__be16			vernum_mask;

	agno = sc->sm->sm_agno;
	if (agno == 0)
		return 0;

	/*
	 * Grab an active reference to the perag structure.  If we can't get
	 * it, we're racing with something that's tearing down the AG, so
	if (!pag)
		return -ENOENT;

	error = xfs_sb_read_secondary(mp, sc->tp, agno, &bp);
	/*
	 * The superblock verifier can return several different error codes
	if (memchr_inv((char *)sb + sblen, 0, BBTOB(bp->b_length) - sblen))
		xchk_block_set_corrupt(sc, bp);

	xchk_superblock_xref(sc, bp);
out_pag:
	xfs_perag_put(pag);
	return error;

	agbno = XFS_AGF_BLOCK(mp);

	xchk_ag_btcur_init(sc, &sc->sa);

	xchk_xref_is_used_space(sc, agbno, 1);
	xchk_agf_xref_freeblks(sc);
	int			level;
	int			error = 0;

	error = xchk_ag_read_headers(sc, agno, &sc->sa);
	if (!xchk_process_error(sc, agno, XFS_AGF_BLOCK(sc->mp), &error))
		goto out;
	xchk_buffer_recheck(sc, sc->sa.agf_bp);

	agbno = XFS_AGFL_BLOCK(mp);

	xchk_ag_btcur_init(sc, &sc->sa);

	xchk_xref_is_used_space(sc, agbno, 1);
	xchk_xref_is_not_inode_chunk(sc, agbno, 1);
	int			error;

	/* Lock the AGF and AGI so that nobody can touch this AG. */
	error = xchk_ag_read_headers(sc, agno, &sc->sa);
	if (!xchk_process_error(sc, agno, XFS_AGFL_BLOCK(sc->mp)
