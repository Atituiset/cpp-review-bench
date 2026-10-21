// AUTO-DRAFT from torvalds/linux PR #75663f575f34291007852859486cafc08dc10d0e
int xchk_setup_fscounters(struct xfs_scrub *sc);
int xchk_setup_nlinks(struct xfs_scrub *sc);
  // <<< BUG ANCHOR
void xchk_ag_free(struct xfs_scrub *sc, struct xchk_ag *sa);
int xchk_ag_init(struct xfs_scrub *sc, xfs_agnumber_t agno,
		struct xchk_ag *sa);
int xchk_perag_drain_and_lock(struct xfs_scrub *sc);

/*
/* …（同文件无关代码省略）… */
static inline int
xchk_ag_init_existing(
	struct xfs_scrub	*sc,
	xfs_agnumber_t		agno,
	struct xchk_ag		*sa)
{
	int			error = xchk_ag_init(sc, agno, sa);

	return error == -ENOENT ? -EFSCORRUPTED : error;
}
/* …（同文件无关代码省略）… */
				 XFS_RTGLOCK_RMAP | \
				 XFS_RTGLOCK_REFCOUNT)

int xchk_rtgroup_init(struct xfs_scrub *sc, xfs_rgnumber_t rgno,
		struct xchk_rt *sr);

static inline int
xchk_rtgroup_init_existing(
	struct xfs_scrub	*sc,
	xfs_rgnumber_t		rgno,
	struct xchk_rt		*sr)
{
	int			error = xchk_rtgroup_init(sc, rgno, sr);

	return error == -ENOENT ? -EFSCORRUPTED : error;
}

int xchk_rtgroup_lock(struct xfs_scrub *sc, struct xchk_rt *sr,
		unsigned int rtglock_flags);
void xchk_rtgroup_unlock(struct xchk_rt *sr);
void xchk_rtgroup_btcur_free(struct xchk_rt *sr);
void xchk_rtgroup_free(struct xfs_scrub *sc, struct xchk_rt *sr);
#else
# define xchk_rtgroup_init(sc, rgno, sr)		(-EFSCORRUPTED)
# define xchk_rtgroup_init_existing(sc, rgno, sr)	(-EFSCORRUPTED)
# define xchk_rtgroup_lock(sc, sr, lockflags)		(-EFSCORRUPTED)
# define xchk_rtgroup_unlock(sr)			do { } while (0)
# define xchk_rtgroup_btcur_free(sr)			do { } while (0)
# define xchk_rtgroup_free(sc, sr)			do { } while (0)
#endif /* CONFIG_XFS_RT */

int xchk_ag_read_headers(struct xfs_scrub *sc, xfs_agnumber_t agno,
		struct xchk_ag *sa);
void xchk_ag_btcur_free(struct xchk_ag *sa);
void xchk_ag_btcur_init(struct xfs_scrub *sc, struct xchk_ag *sa);
int xchk_count_rmap_ownedby_ag(struct xfs_scrub *sc, struct xfs_btree_cur *cur,
		const struct xfs_owner_info *oinfo, xfs_filblks_t *blocks);
