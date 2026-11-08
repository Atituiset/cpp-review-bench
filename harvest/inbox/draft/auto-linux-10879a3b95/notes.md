# auto-linux-10879a3b95

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#fa4ad798f6f681031775266044ba1b0cf3c670bd](https://github.com/torvalds/linux/commit/fa4ad798f6f681031775266044ba1b0cf3c670bd) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-08 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 5 |
| 编译错误数（gcc syntax-only） | 9（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #fa4ad798f6f681031775266044ba1b0cf3c670bd (https://github.com/torvalds/linux/commit/fa4ad798f6f681031775266044ba1b0cf3c670bd)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 251；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT fa4ad798f6f681031775266044ba1b0cf3c670bd Merge tag 'for-7.4-rc1-tag' of git://git.kernel.org/pub/scm/linux/kernel/git/kdave/linux :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -242,13 +242,22 @@ static int btrfs_run_defrag_inode(struct btrfs_fs_info *fs_info,
 		goto cleanup;
 	}
 
+	/*
+	 * Clear the flag right away. On the first iteration (cur == 0) if the
+	 * inode happened to have been truncated to 0 after it was added to
+	 * autodefrag and before we were called, then we need to clear the flag,
+	 * otherwise if writes are done to the inode we won't be able to add it
+	 * to autodefrag again (unless the inode is evicted, loaded again, and
+	 * small writes to it happen again).
+	 */
+	clear_bit(BTRFS_INODE_IN_DEFRAG, &inode->runtime_flags);
+
 	if (cur >= i_size_read(&inode->vfs_inode)) {
 		iput(&inode->vfs_inode);
 		goto cleanup;
 	}
 
 	/* Do a chunk of defrag */
-	clear_bit(BTRFS_INODE_IN_DEFRAG, &inode->runtime_flags);
 	memset(&range, 0, sizeof(range));
 	range.len = (u64)-1;
 	range.start = cur;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`clear_bit`
- 外部函数：`i_size_read`
- 外部函数：`iput`
- 外部函数：`memset`
- 大写宏：`BTRFS_INODE_IN_DEFRAG`
- 外部类型：`Do`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-linux-10879a3b95` → 本草稿移入 `cases/defect/auto-linux-10879a3b95/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
