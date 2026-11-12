# auto-linux-0d734bf539

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#9d66f26fefacebfeed15de31c1123451a17ff119](https://github.com/torvalds/linux/commit/9d66f26fefacebfeed15de31c1123451a17ff119) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-12 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 22 |
| 编译错误数（gcc syntax-only） | 18（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #9d66f26fefacebfeed15de31c1123451a17ff119 (https://github.com/torvalds/linux/commit/9d66f26fefacebfeed15de31c1123451a17ff119)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 9d66f26fefacebfeed15de31c1123451a17ff119 Merge tag 'vfs-7.4-rc3.fixes.2' of git://git.kernel.org/pub/scm/linux/kernel/git/vfs/vfs :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -621,12 +621,15 @@ void write_boundary_block(struct block_device *bdev,
  * @bh: The buffer to mark dirty.
  * @mmb: The list of buffers to add the buffer to.
  *
- * Mark the buffer dirty and add it to the list if it is not already on
- * a list.
+ * Mark the buffer dirty, add it to the list if it is not already on a list
+ * and set I_METADATA_WRITEBACK on the inode so that fsync() and eviction
+ * write it out.
  */
 void mmb_mark_buffer_dirty(struct buffer_head *bh,
 			   struct mapping_metadata_bhs *mmb)
 {
+	struct inode *inode = mmb->mapping->host;
+
 	mark_buffer_dirty(bh);
 	if (!bh->b_mmb) {
 		spin_lock(&mmb->lock);
@@ -641,6 +644,15 @@ void mmb_mark_buffer_dirty(struct buffer_head *bh,
 		list_move_tail(&bh->b_assoc_buffers, &mmb->list);
 		spin_unlock(&mmb->lock);
 	}
+	/*
+	 * The inode itself may never have been dirtied, so attach it to a wb
+	 * before setting the bit or writeback_single_inode() has nothing to
+	 * lock when evict() syncs it.
+	 */
+	spin_lock(&inode->i_lock);
+	inode_attach_wb(inode, NULL);
+	inode_state_set(inode, I_METADATA_WRITEBACK);
+	spin_unlock(&inode->i_lock);
 }
 EXPORT_SYMBOL(mmb_mark_buffer_dirty);
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__folio_mark_dirty`
- 外部函数：`__mark_inode_dirty`
- 外部函数：`buffer_dirty`
- 外部函数：`buffer_uptodate`
- 外部函数：`folio_test_set_dirty`
- 外部函数：`list_move_tail`
- 外部函数：`smp_mb`
- 外部函数：`spin_lock`
- 外部函数：`spin_unlock`
- 外部函数：`test_set_buffer_dirty`
- 外部函数：`trace_block_dirty_buffer`
- 大写宏：`EXPORT_SYMBOL`
- 大写宏：`I_DIRTY_PAGES`
- 大写宏：`NULL`
- 大写宏：`WARN_ON_ONCE`
- 外部类型：`Don`
- 外部类型：`Mark`
- 外部类型：`The`
- 外部类型：`Very`
- 外部类型：`address_space`
- 外部类型：`buffer_head`
- 外部类型：`folio`
- 外部类型：`mapping_metadata_bhs`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-0d734bf539` → 本草稿移入 `cases/defect/auto-linux-0d734bf539/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
