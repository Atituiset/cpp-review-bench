# auto-linux-4178083faf

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#75663f575f34291007852859486cafc08dc10d0e](https://github.com/torvalds/linux/commit/75663f575f34291007852859486cafc08dc10d0e) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-22 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 37 |
| 编译错误数（gcc syntax-only） | 19（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #75663f575f34291007852859486cafc08dc10d0e (https://github.com/torvalds/linux/commit/75663f575f34291007852859486cafc08dc10d0e)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 38；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 75663f575f34291007852859486cafc08dc10d0e Merge tag 'xfs-merge-7.4' of git://git.kernel.org:/pub/scm/fs/xfs/xfs-linux :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -35,8 +35,7 @@ xchk_setup_agheader(
 /* Cross-reference with the other btrees. */
 STATIC void
 xchk_superblock_xref(
-	struct xfs_scrub	*sc,
-	struct xfs_buf		*bp)
+	struct xfs_scrub	*sc)
 {
 	struct xfs_mount	*mp = sc->mp;
 	xfs_agnumber_t		agno = sc->sm->sm_agno;
@@ -48,7 +47,7 @@ xchk_superblock_xref(
 
 	agbno = XFS_SB_BLOCK(mp);
 
-	error = xchk_ag_init_existing(sc, agno, &sc->sa);
+	error = xchk_ag_init_existing(sc, agno);
 	if (!xchk_xref_process_error(sc, agno, agbno, &error))
 		return;
 
@@ -57,8 +56,7 @@ xchk_superblock_xref(
 	xchk_xref_is_only_owned_by(sc, agbno, 1, &XFS_RMAP_OINFO_FS);
 	xchk_xref_is_not_shared(sc, agbno, 1);
 	xchk_xref_is_not_cow_staging(sc, agbno, 1);
-
-	/* scrub teardown will take care of sc->sa for us */
+	xchk_ag_free(sc);
 }
 
 /*
@@ -106,16 +104,12 @@ xchk_superblock(
 	struct xfs_dsb		*sb;
 	struct xfs_perag	*pag;
 	size_t			sblen;
-	xfs_agnumber_t		agno;
+	xfs_agnumber_t		agno = sc->sm->sm_agno;
 	uint32_t		v2_ok;
 	__be32			features_mask;
 	int			error;
 	__be16			vernum_mask;
 
-	agno = sc->sm->sm_agno;
-	if (agno == 0)
-		return 0;
-
 	/*
 	 * Grab an active reference to the perag structure.  If we can't get
 	 * it, we're racing with something that's tearing down the AG, so
@@ -125,6 +119,41 @@ xchk_superblock(
 	if (!pag)
 		return -ENOENT;
 
+	if (agno == 0) {
+		/*
+		 * Reread the primary super from disk in case it's become
+		 * corrupted enough since mount time to fail the verifier.
+		 * Crashing now with a bad primary super will prevent remount
+		 * (which might recover the primary super from the log) so we
+		 * want to trigger a repair to write the incore primary
+		 * superblock out to disk ASAP.
+		 *
+		 * Note that we don't check the geometry (like we do for a
+		 * secondary super) because the incore copy is the source of
+		 * truth while the filesystem is mounted.
+		 *
+		 * If the read races with a write, we end up writing the
+		 * primary super unnecessarily, but that's not a big deal.
+		 */
+		error = xfs_buf_read_uncached(sc->mp->m_ddev_targp,
+				XFS_SB_DADDR, BTOBB(mp->m_sb.sb_sectsize), &bp,
+				&xfs_sb_buf_ops);
+		switch (error) {
+		case -EINVAL:	/* also -EWRONGFS */
+		case -ENOSYS:
+		case -EFBIG:
+			error = -EFSCORRUPTED;
+			fallthrough;
+		default:
+			break;
+		}
+		if (!xchk_process_error(sc, agno, XFS_SB_BLOCK(mp), &error))
+			goto out_pag;
+
+		xfs_buf_relse(bp);
+		goto out_xref;
+	}
+
 	error = xfs_sb_read_secondary(mp, sc->tp, agno, &bp);
 	/*
 	 * The superblock verifier can return several different error codes
@@ -430,7 +459,8 @@ xchk_superblock(
 	if (memchr_inv((char *)sb + sblen, 0, BBTOB(bp->b_length) - sblen))
 		xchk_block_set_corrupt(sc, bp);
 
-	xchk_superblock_xref(sc, bp);
+out_xref:
+	xchk_superblock_xref(sc);
 out_pag:
 	xfs_perag_put(pag);
 	return error;
@@ -585,7 +615,7 @@ xchk_agf_xref(
 
 	agbno = XFS_AGF_BLOCK(mp);
 
-	xchk_ag_btcur_init(sc, &sc->sa);
+	xchk_ag_btcur_init(sc);
 
 	xchk_xref_is_used_space(sc, agbno, 1);
 	xchk_agf_xref_freeblks(sc);
@@ -618,7 +648,7 @@ xchk_agf(
 	int			level;
 	int			error = 0;
 
-	error = xchk_ag_read_headers(sc, agno, &sc->sa);
+	error = xchk_ag_read_headers(sc, agno);
 	if (!xchk_process_error(sc, agno, XFS_AGF_BLOCK(sc->mp), &error))
 		goto out;
 	xchk_buffer_recheck(sc, sc->sa.agf_bp);
@@ -773,7 +803,7 @@ xchk_agfl_xref(
 
 	agbno = XFS_AGFL_BLOCK(mp);
 
-	xchk_ag_btcur_init(sc, &sc->sa);
+	xchk_ag_btcur_init(sc);
 
 	xchk_xref_is_used_space(sc, agbno, 1);
 	xchk_xref_is_not_inode_chunk(sc, agbno, 1);
@@ -801,7 +831,7 @@ xchk_agfl(
 	int			error;
 
 	/* Lock the AGF and AGI so that nobody can touch this AG. */
-	error = xchk_ag_read_headers(sc, agno, &sc->sa);
+	error = xchk_ag_read_headers(sc, agno);
 	if (!xchk_process_error(sc, agno, XFS_AGFL_BLOCK(sc->mp), &error))
 		return error;
 	if (!sc->sa.agf_bp)
@@ -928,7 +958,7 @@ xchk_agi_xref(
 
 	agbno = XFS_AGI_BLOCK(mp);
 
-	xchk_ag_btcur_init(sc, &sc->sa);
+	xchk_ag_btcur_init(sc);
 
 	xchk_xref_is_used_space(sc, agbno, 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`memchr_inv`
- 外部函数：`xchk_ag_btcur_init`
- 外部函数：`xchk_ag_init_existing`
- 外部函数：`xchk_ag_read_headers`
- 外部函数：`xchk_agf_xref_freeblks`
- 外部函数：`xchk_block_set_corrupt`
- 外部函数：`xchk_buffer_recheck`
- 外部函数：`xchk_process_error`
- 外部函数：`xchk_xref_is_not_cow_staging`
- 外部函数：`xchk_xref_is_not_inode_chunk`
- 外部函数：`xchk_xref_is_not_shared`
- 外部函数：`xchk_xref_is_only_owned_by`
- 外部函数：`xchk_xref_is_used_space`
- 外部函数：`xchk_xref_process_error`
- 外部函数：`xfs_perag_put`
- 外部函数：`xfs_sb_read_secondary`
- 大写宏：`AGF`
- 大写宏：`AGI`
- 大写宏：`BBTOB`
- 大写宏：`ENOENT`
- 大写宏：`STATIC`
- 大写宏：`XFS_AGFL_BLOCK`
- 大写宏：`XFS_AGF_BLOCK`
- 大写宏：`XFS_RMAP_OINFO_FS`
- 大写宏：`XFS_SB_BLOCK`
- 外部类型：`Cross`
- 外部类型：`Grab`
- 外部类型：`If`
- 外部类型：`Lock`
- 外部类型：`The`
- 外部类型：`size_t`
- 外部类型：`uint32_t`
- 外部类型：`xfs_agnumber_t`
- 外部类型：`xfs_buf`
- 外部类型：`xfs_dsb`
- 外部类型：`xfs_mount`
- 外部类型：`xfs_perag`
- 外部类型：`xfs_scrub`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。
- **依赖重（dep_count≥10）**：可考虑只做 PR/diff 形态评审，不做独立 case。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-linux-4178083faf` → 本草稿移入 `cases/defect/auto-linux-4178083faf/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
