# auto-linux-1a2b63e9a3

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#75663f575f34291007852859486cafc08dc10d0e](https://github.com/torvalds/linux/commit/75663f575f34291007852859486cafc08dc10d0e) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-21 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 40 |
| 编译错误数（gcc syntax-only） | 7（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #75663f575f34291007852859486cafc08dc10d0e (https://github.com/torvalds/linux/commit/75663f575f34291007852859486cafc08dc10d0e)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 678；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 75663f575f34291007852859486cafc08dc10d0e Merge tag 'xfs-merge-7.4' of git://git.kernel.org:/pub/scm/fs/xfs/xfs-linux :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -107,8 +107,9 @@ __xchk_process_error(
 	case -ENODATA:
 		/* Note the badness but don't abort. */
 		sc->sm->sm_flags |= errflag;
+		trace_xchk_op_error(sc, agno, bno, *error, ret_ip);
 		*error = 0;
-		fallthrough;
+		break;
 	default:
 		trace_xchk_op_error(sc, agno, bno, *error, ret_ip);
 		break;
@@ -183,8 +184,10 @@ __xchk_fblock_process_error(
 	case -ENODATA:
 		/* Note the badness but don't abort. */
 		sc->sm->sm_flags |= errflag;
+		trace_xchk_file_op_error(sc, whichfork, offset, *error,
+				ret_ip);
 		*error = 0;
-		fallthrough;
+		break;
 	default:
 		trace_xchk_file_op_error(sc, whichfork, offset, *error,
 				ret_ip);
@@ -575,10 +578,10 @@ xchk_perag_drain_and_lock(
 int
 xchk_ag_read_headers(
 	struct xfs_scrub	*sc,
-	xfs_agnumber_t		agno,
-	struct xchk_ag		*sa)
+	xfs_agnumber_t		agno)
 {
 	struct xfs_mount	*mp = sc->mp;
+	struct xchk_ag		*sa = &sc->sa;
 
 	ASSERT(!sa->pag);
 	sa->pag = xfs_perag_get(mp, agno);
@@ -591,8 +594,10 @@ xchk_ag_read_headers(
 /* Release all the AG btree cursors. */
 void
 xchk_ag_btcur_free(
-	struct xchk_ag		*sa)
+	struct xfs_scrub	*sc)
 {
+	struct xchk_ag		*sa = &sc->sa;
+
 	if (sa->refc_cur)
 		xfs_btree_del_cursor(sa->refc_cur, XFS_BTREE_ERROR);
 	if (sa->rmap_cur)
@@ -617,10 +622,10 @@ xchk_ag_btcur_free(
 /* Initialize all the btree cursors for an AG. */
 void
 xchk_ag_btcur_init(
-	struct xfs_scrub	*sc,
-	struct xchk_ag		*sa)
+	struct xfs_scrub	*sc)
 {
 	struct xfs_mount	*mp = sc->mp;
+	struct xchk_ag		*sa = &sc->sa;
 
 	if (sa->agf_bp) {
 		/* Set up a bnobt cursor for cross-referencing. */
@@ -672,10 +677,11 @@ xchk_ag_btcur_init(
 /* Release the AG header context and btree cursors. */
 void
 xchk_ag_free(
-	struct xfs_scrub	*sc,
-	struct xchk_ag		*sa)
+	struct xfs_scrub	*sc)
 {
-	xchk_ag_btcur_free(sa);
+	struct xchk_ag		*sa = &sc->sa;
+
+	xchk_ag_btcur_free(sc);
 	xrep_reset_perag_resv(sc);
 	if (sa->agf_bp) {
 		xfs_trans_brelse(sc->tp, sa->agf_bp);
@@ -701,16 +707,15 @@ xchk_ag_free(
 int
 xchk_ag_init(
 	struct xfs_scrub	*sc,
-	xfs_agnumber_t		agno,
-	struct xchk_ag		*sa)
+	xfs_agnumber_t		agno)
 {
 	int			error;
 
-	error = xchk_ag_read_headers(sc, agno, sa);
+	error = xchk_ag_read_headers(sc, agno);
 	if (error)
 		return error;
 
-	xchk_ag_btcur_init(sc, sa);
+	xchk_ag_btcur_init(sc);
 	return 0;
 }
 
@@ -725,9 +730,10 @@ xchk_ag_init(
 int
 xchk_rtgroup_init(
 	struct xfs_scrub	*sc,
-	xfs_rgnumber_t		rgno,
-	struct xchk_rt		*sr)
+	xfs_rgnumber_t		rgno)
 {
+	struct xchk_rt		*sr = &sc->sr;
+
 	ASSERT(sr->rtg == NULL);
 	ASSERT(sr->rtlock_flags == 0);
 
@@ -741,9 +747,9 @@ xchk_rtgroup_init(
 int
 xchk_rtgroup_lock(
 	struct xfs_scrub	*sc,
-	struct xchk_rt		*sr,
 	unsigned int		rtglock_flags)
 {
+	struct xchk_rt		*sr = &sc->sr;
 	int			error = 0;
 
 	ASSERT(sr->rtg != NULL);
@@ -826,8 +832,10 @@ xchk_rtgroup_lock(
  */
 void
 xchk_rtgroup_btcur_free(
-	struct xchk_rt		*sr)
+	struct xfs_scrub	*sc)
 {
+	struct xchk_rt		*sr = &sc->sr;
+
 	if (sr->rmap_cur)
 		xfs_btree_del_cursor(sr->rmap_cur, XFS_BTREE_ERROR);
 	if (sr->refc_cur)
@@ -843,8 +851,10 @@ xchk_rtgroup_btcur_free(
  */
 void
 xchk_rtgroup_unlock(
-	struct xchk_rt		*sr)
+	struct xfs_scrub	*sc)
 {
+	struct xchk_rt		*sr = &sc->sr;
+
 	ASSERT(sr->rtg != NULL);
 
 	if (sr->rtlock_flags) {
@@ -859,12 +869,13 @@ xchk_rtgroup_unlock(
  */
 void
 xchk_rtgroup_free(
-	struct xfs_scrub	*sc,
-	struct xchk_rt		*sr)
+	struct xfs_scrub	*sc)
 {
+	struct xchk_rt		*sr = &sc->sr;
+
 	ASSERT(sr->rtg != NULL);
 
-	xchk_rtgroup_unlock(sr);
+	xchk_rtgroup_unlock(sc);
 
 	xfs_rtgroup_put(sr->rtg);
 	sr->rtg = NULL;
@@ -955,7 +966,7 @@ xchk_setup_ag_btree(
 	if (error)
 		return error;
 
-	return xchk_ag_init(sc, sc->sm->sm_agno, &sc->sa);
+	return xchk_ag_init(sc, sc->sm->sm_agno);
 }
 
 /* Push everything out of the log onto disk. */
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`trace_xchk_file_op_error`
- 外部函数：`trace_xchk_op_error`
- 外部函数：`xchk_ag_btcur_init`
- 外部函数：`xchk_ag_btree_del_cursor_if_sick`
- 外部函数：`xchk_rtgroup_unlock`
- 外部函数：`xfs_bnobt_init_cursor`
- 外部函数：`xfs_btree_del_cursor`
- 外部函数：`xfs_cntbt_init_cursor`
- 外部函数：`xfs_finobt_init_cursor`
- 外部函数：`xfs_has_finobt`
- 外部函数：`xfs_has_reflink`
- 外部函数：`xfs_has_rmapbt`
- 外部函数：`xfs_inobt_init_cursor`
- 外部函数：`xfs_perag_get`
- 外部函数：`xfs_refcountbt_init_cursor`
- 外部函数：`xfs_rmapbt_init_cursor`
- 外部函数：`xfs_rtgroup_put`
- 外部函数：`xfs_rtgroup_unlock`
- 外部函数：`xfs_trans_brelse`
- 外部函数：`xrep_reset_perag_resv`
- 大写宏：`ASSERT`
- 大写宏：`ENODATA`
- 大写宏：`NULL`
- 大写宏：`XFS_BTREE_ERROR`
- 大写宏：`XFS_SCRUB_TYPE_BNOBT`
- 大写宏：`XFS_SCRUB_TYPE_CNTBT`
- 大写宏：`XFS_SCRUB_TYPE_FINOBT`
- 大写宏：`XFS_SCRUB_TYPE_INOBT`
- 大写宏：`XFS_SCRUB_TYPE_REFCNTBT`
- 大写宏：`XFS_SCRUB_TYPE_RMAPBT`
- 外部类型：`Initialize`
- 外部类型：`Note`
- 外部类型：`Push`
- 外部类型：`Release`
- 外部类型：`Set`
- 外部类型：`xchk_ag`
- 外部类型：`xchk_rt`
- 外部类型：`xfs_agnumber_t`
- 外部类型：`xfs_mount`
- 外部类型：`xfs_rgnumber_t`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-1a2b63e9a3` → 本草稿移入 `cases/defect/auto-linux-1a2b63e9a3/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
