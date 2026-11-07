# auto-linux-7f517abf82

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#0ccc72d4b3fcf8c79178b7024541e1aa694e08a7](https://github.com/torvalds/linux/commit/0ccc72d4b3fcf8c79178b7024541e1aa694e08a7) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-07 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 12 |
| 编译错误数（gcc syntax-only） | 12（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #0ccc72d4b3fcf8c79178b7024541e1aa694e08a7 (https://github.com/torvalds/linux/commit/0ccc72d4b3fcf8c79178b7024541e1aa694e08a7)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 75；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 0ccc72d4b3fcf8c79178b7024541e1aa694e08a7 Merge tag 'xfs-fixes-7.4-rc2' of git://git.kernel.org:/pub/scm/fs/xfs/xfs-linux :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -72,7 +72,7 @@ xfs_trans_free(
 	xfs_extent_busy_clear(&tp->t_busy, false);
 
 	trace_xfs_trans_free(tp, _RET_IP_);
-	xfs_trans_clear_context(tp);
+	memalloc_nofs_restore(tp->t_pflags);
 	if (!(tp->t_flags & XFS_TRANS_NO_WRITECOUNT))
 		sb_end_intwrite(tp->t_mountp->m_super);
 	xfs_trans_free_dqinfo(tp);
@@ -220,7 +220,11 @@ __xfs_trans_alloc(
 	tp = kmem_cache_zalloc(xfs_trans_cache, GFP_KERNEL | __GFP_NOFAIL);
 	if (!(flags & XFS_TRANS_NO_WRITECOUNT))
 		sb_start_intwrite(mp->m_super);
-	xfs_trans_set_context(tp);
+	/*
+	 * Save NOFS state for xfs_trans_free() to restore. Only the
+	 * transaction owner thread may touch tp->t_pflags.
+	 */
+	tp->t_pflags = memalloc_nofs_save();
 	tp->t_flags = flags;
 	tp->t_mountp = mp;
 	INIT_LIST_HEAD(&tp->t_items);
@@ -1038,7 +1042,7 @@ xfs_trans_roll(
 	 * xfs_trans_free.  Set it again here before doing memory
 	 * allocations.
 	 */
-	xfs_trans_set_context(tp);
+	tp->t_pflags = memalloc_nofs_save();
 
 	if (error)
 		return error;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`kmem_cache_zalloc`
- 外部函数：`sb_end_intwrite`
- 外部函数：`sb_start_intwrite`
- 外部函数：`trace_xfs_trans_free`
- 外部函数：`xfs_extent_busy_clear`
- 外部函数：`xfs_trans_clear_context`
- 外部函数：`xfs_trans_free_dqinfo`
- 外部函数：`xfs_trans_set_context`
- 大写宏：`GFP_KERNEL`
- 大写宏：`INIT_LIST_HEAD`
- 大写宏：`XFS_TRANS_NO_WRITECOUNT`
- 外部类型：`Set`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-7f517abf82` → 本草稿移入 `cases/defect/auto-linux-7f517abf82/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
