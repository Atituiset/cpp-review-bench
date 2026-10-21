// AUTO-DRAFT from torvalds/linux PR #75663f575f34291007852859486cafc08dc10d0e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	case -ENODATA:
		/* Note the badness but don't abort. */
		sc->sm->sm_flags |= errflag;
		*error = 0;
		fallthrough;
	default:
		trace_xchk_op_error(sc, agno, bno, *error, ret_ip);
		break;
/* …（同文件无关代码省略）… */
	case -ENODATA:
		/* Note the badness but don't abort. */
		sc->sm->sm_flags |= errflag;
		*error = 0;
		fallthrough;
	default:
		trace_xchk_file_op_error(sc, whichfork, offset, *error,
				ret_ip);
/* …（同文件无关代码省略）… */
int
xchk_ag_read_headers(
	struct xfs_scrub	*sc,
	xfs_agnumber_t		agno,
	struct xchk_ag		*sa)
{
	struct xfs_mount	*mp = sc->mp;

	ASSERT(!sa->pag);
	sa->pag = xfs_perag_get(mp, agno);
/* …（同文件无关代码省略）… */
/* Release all the AG btree cursors. */
void
xchk_ag_btcur_free(
	struct xchk_ag		*sa)
{
	if (sa->refc_cur)
		xfs_btree_del_cursor(sa->refc_cur, XFS_BTREE_ERROR);
	if (sa->rmap_cur)
/* …（同文件无关代码省略）… */
/* Initialize all the btree cursors for an AG. */
void
xchk_ag_btcur_init(
	struct xfs_scrub	*sc,
	struct xchk_ag		*sa)
{
	struct xfs_mount	*mp = sc->mp;

	if (sa->agf_bp) {
		/* Set up a bnobt cursor for cross-referencing. */
		sa->bno_cur = xfs_bnobt_init_cursor(mp, sc->tp, sa->agf_bp,
				sa->pag);
		xchk_ag_btree_del_cursor_if_sick(sc, &sa->bno_cur,
				XFS_SCRUB_TYPE_BNOBT);

		/* Set up a cntbt cursor for cross-referencing. */
		sa->cnt_cur = xfs_cntbt_init_cursor(mp, sc->tp, sa->agf_bp,
				sa->pag);
		xchk_ag_btree_del_cursor_if_sick(sc, &sa->cnt_cur,
				XFS_SCRUB_TYPE_CNTBT);

		/* Set up a rmapbt cursor for cross-referencing. */
		if (xfs_has_rmapbt(mp)) {
			sa->rmap_cur = xfs_rmapbt_init_cursor(mp, sc->tp,
					sa->agf_bp, sa->pag);
			xchk_ag_btree_del_cursor_if_sick(sc, &sa->rmap_cur,
					XFS_SCRUB_TYPE_RMAPBT);
		}

		/* Set up a refcountbt cursor for cross-referencing. */
		if (xfs_has_reflink(mp)) {
			sa->refc_cur = xfs_refcountbt_init_cursor(mp, sc->tp,
					sa->agf_bp, sa->pag);
			xchk_ag_btree_del_cursor_if_sick(sc, &sa->refc_cur,
					XFS_SCRUB_TYPE_REFCNTBT);
		}
	}

	if (sa->agi_bp) {
		/* Set up a inobt cursor for cross-referencing. */
		sa->ino_cur = xfs_inobt_init_cursor(sa->pag, sc->tp,
				sa->agi_bp);
		xchk_ag_btree_del_cursor_if_sick(sc, &sa->ino_cur,
				XFS_SCRUB_TYPE_INOBT);

		/* Set up a finobt cursor for cross-referencing. */
		if (xfs_has_finobt(mp)) {
			sa->fino_cur = xfs_finobt_init_cursor(sa->pag, sc->tp,
					sa->agi_bp);
			xchk_ag_btree_del_cursor_if_sick(sc, &sa->fino_cur,
					XFS_SCRUB_TYPE_FINOBT);
		}
	}
}
/* …（同文件无关代码省略）… */
/* Release the AG header context and btree cursors. */
void
xchk_ag_free(
	struct xfs_scrub	*sc,
	struct xchk_ag		*sa)
{
	xchk_ag_btcur_free(sa);
	xrep_reset_perag_resv(sc);
	if (sa->agf_bp) {
		xfs_trans_brelse(sc->tp, sa->agf_bp);
/* …（同文件无关代码省略）… */
int
xchk_ag_init(
	struct xfs_scrub	*sc,
	xfs_agnumber_t		agno,
	struct xchk_ag		*sa)
{
	int			error;

	error = xchk_ag_read_headers(sc, agno, sa);
	if (error)
		return error;

	xchk_ag_btcur_init(sc, sa);
	return 0;
}

/* …（同文件无关代码省略）… */
int
xchk_rtgroup_init(
	struct xfs_scrub	*sc,
	xfs_rgnumber_t		rgno,
	struct xchk_rt		*sr)
{
	ASSERT(sr->rtg == NULL);
	ASSERT(sr->rtlock_flags == 0);

/* …（同文件无关代码省略）… */
int
xchk_rtgroup_lock(
	struct xfs_scrub	*sc,
	struct xchk_rt		*sr,
	unsigned int		rtglock_flags)
{
	int			error = 0;

	ASSERT(sr->rtg != NULL);
/* …（同文件无关代码省略）… */
 */
void
xchk_rtgroup_btcur_free(
	struct xchk_rt		*sr)
{
	if (sr->rmap_cur)
		xfs_btree_del_cursor(sr->rmap_cur, XFS_BTREE_ERROR);
	if (sr->refc_cur)
/* …（同文件无关代码省略）… */
 */
void
xchk_rtgroup_unlock(
	struct xchk_rt		*sr)
{
	ASSERT(sr->rtg != NULL);

	if (sr->rtlock_flags) {
		xfs_rtgroup_unlock(sr->rtg, sr->rtlock_flags);
		sr->rtlock_flags = 0;
	}
}
/* …（同文件无关代码省略）… */
 */
void
xchk_rtgroup_free(
	struct xfs_scrub	*sc,
	struct xchk_rt		*sr)
{
	ASSERT(sr->rtg != NULL);

	xchk_rtgroup_unlock(sr);

	xfs_rtgroup_put(sr->rtg);
	sr->rtg = NULL;
/* …（同文件无关代码省略）… */
	if (error)
		return error;

	return xchk_ag_init(sc, sc->sm->sm_agno, &sc->sa);
}

/* Push everything out of the log onto disk. */
