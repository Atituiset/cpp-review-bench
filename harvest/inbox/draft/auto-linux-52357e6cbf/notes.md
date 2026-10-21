# auto-linux-52357e6cbf

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
| 外部依赖数（dep_count） | 27 |
| 编译错误数（gcc syntax-only） | 8（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #75663f575f34291007852859486cafc08dc10d0e (https://github.com/torvalds/linux/commit/75663f575f34291007852859486cafc08dc10d0e)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 112；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 75663f575f34291007852859486cafc08dc10d0e Merge tag 'xfs-merge-7.4' of git://git.kernel.org:/pub/scm/fs/xfs/xfs-linux :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -109,9 +109,8 @@ xchk_ino_dqattach(struct xfs_scrub *sc)
 int xchk_setup_fscounters(struct xfs_scrub *sc);
 int xchk_setup_nlinks(struct xfs_scrub *sc);
 
-void xchk_ag_free(struct xfs_scrub *sc, struct xchk_ag *sa);
-int xchk_ag_init(struct xfs_scrub *sc, xfs_agnumber_t agno,
-		struct xchk_ag *sa);
+void xchk_ag_free(struct xfs_scrub *sc);
+int xchk_ag_init(struct xfs_scrub *sc, xfs_agnumber_t agno);
 int xchk_perag_drain_and_lock(struct xfs_scrub *sc);
 
 /*
@@ -122,10 +121,9 @@ int xchk_perag_drain_and_lock(struct xfs_scrub *sc);
 static inline int
 xchk_ag_init_existing(
 	struct xfs_scrub	*sc,
-	xfs_agnumber_t		agno,
-	struct xchk_ag		*sa)
+	xfs_agnumber_t		agno)
 {
-	int			error = xchk_ag_init(sc, agno, sa);
+	int			error = xchk_ag_init(sc, agno);
 
 	return error == -ENOENT ? -EFSCORRUPTED : error;
 }
@@ -137,38 +135,34 @@ xchk_ag_init_existing(
 				 XFS_RTGLOCK_RMAP | \
 				 XFS_RTGLOCK_REFCOUNT)
 
-int xchk_rtgroup_init(struct xfs_scrub *sc, xfs_rgnumber_t rgno,
-		struct xchk_rt *sr);
+int xchk_rtgroup_init(struct xfs_scrub *sc, xfs_rgnumber_t rgno);
 
 static inline int
 xchk_rtgroup_init_existing(
 	struct xfs_scrub	*sc,
-	xfs_rgnumber_t		rgno,
-	struct xchk_rt		*sr)
+	xfs_rgnumber_t		rgno)
 {
-	int			error = xchk_rtgroup_init(sc, rgno, sr);
+	int			error = xchk_rtgroup_init(sc, rgno);
 
 	return error == -ENOENT ? -EFSCORRUPTED : error;
 }
 
-int xchk_rtgroup_lock(struct xfs_scrub *sc, struct xchk_rt *sr,
-		unsigned int rtglock_flags);
-void xchk_rtgroup_unlock(struct xchk_rt *sr);
-void xchk_rtgroup_btcur_free(struct xchk_rt *sr);
-void xchk_rtgroup_free(struct xfs_scrub *sc, struct xchk_rt *sr);
+int xchk_rtgroup_lock(struct xfs_scrub *sc, unsigned int rtglock_flags);
+void xchk_rtgroup_unlock(struct xfs_scrub *sc);
+void xchk_rtgroup_btcur_free(struct xfs_scrub *sc);
+void xchk_rtgroup_free(struct xfs_scrub *sc);
 #else
-# define xchk_rtgroup_init(sc, rgno, sr)		(-EFSCORRUPTED)
-# define xchk_rtgroup_init_existing(sc, rgno, sr)	(-EFSCORRUPTED)
-# define xchk_rtgroup_lock(sc, sr, lockflags)		(-EFSCORRUPTED)
-# define xchk_rtgroup_unlock(sr)			do { } while (0)
-# define xchk_rtgroup_btcur_free(sr)			do { } while (0)
-# define xchk_rtgroup_free(sc, sr)			do { } while (0)
+# define xchk_rtgroup_init(sc, rgno)			(-EFSCORRUPTED)
+# define xchk_rtgroup_init_existing(sc, rgno)		(-EFSCORRUPTED)
+# define xchk_rtgroup_lock(sc, lockflags)		(-EFSCORRUPTED)
+# define xchk_rtgroup_unlock(sc)			do { } while (0)
+# define xchk_rtgroup_btcur_free(sc)			do { } while (0)
+# define xchk_rtgroup_free(sc)				do { } while (0)
 #endif /* CONFIG_XFS_RT */
 
-int xchk_ag_read_headers(struct xfs_scrub *sc, xfs_agnumber_t agno,
-		struct xchk_ag *sa);
-void xchk_ag_btcur_free(struct xchk_ag *sa);
-void xchk_ag_btcur_init(struct xfs_scrub *sc, struct xchk_ag *sa);
+int xchk_ag_read_headers(struct xfs_scrub *sc, xfs_agnumber_t agno);
+void xchk_ag_btcur_free(struct xfs_scrub *sc);
+void xchk_ag_btcur_init(struct xfs_scrub *sc);
 int xchk_count_rmap_ownedby_ag(struct xfs_scrub *sc, struct xfs_btree_cur *cur,
 		const struct xfs_owner_info *oinfo, xfs_filblks_t *blocks);
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`xchk_ag_btcur_free`
- 外部函数：`xchk_ag_btcur_init`
- 外部函数：`xchk_ag_free`
- 外部函数：`xchk_ag_init`
- 外部函数：`xchk_ag_read_headers`
- 外部函数：`xchk_count_rmap_ownedby_ag`
- 外部函数：`xchk_perag_drain_and_lock`
- 外部函数：`xchk_rtgroup_btcur_free`
- 外部函数：`xchk_rtgroup_free`
- 外部函数：`xchk_rtgroup_init`
- 外部函数：`xchk_rtgroup_lock`
- 外部函数：`xchk_rtgroup_unlock`
- 外部函数：`xchk_setup_fscounters`
- 外部函数：`xchk_setup_nlinks`
- 大写宏：`CONFIG_XFS_RT`
- 大写宏：`EFSCORRUPTED`
- 大写宏：`ENOENT`
- 大写宏：`XFS_RTGLOCK_REFCOUNT`
- 大写宏：`XFS_RTGLOCK_RMAP`
- 外部类型：`xchk_ag`
- 外部类型：`xchk_rt`
- 外部类型：`xfs_agnumber_t`
- 外部类型：`xfs_btree_cur`
- 外部类型：`xfs_filblks_t`
- 外部类型：`xfs_owner_info`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-52357e6cbf` → 本草稿移入 `cases/defect/auto-linux-52357e6cbf/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
