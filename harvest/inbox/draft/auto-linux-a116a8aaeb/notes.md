# auto-linux-a116a8aaeb

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
| 外部依赖数（dep_count） | 8 |
| 编译错误数（gcc syntax-only） | 7（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #fa4ad798f6f681031775266044ba1b0cf3c670bd (https://github.com/torvalds/linux/commit/fa4ad798f6f681031775266044ba1b0cf3c670bd)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT fa4ad798f6f681031775266044ba1b0cf3c670bd Merge tag 'for-7.4-rc1-tag' of git://git.kernel.org/pub/scm/linux/kernel/git/kdave/linux :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1847,6 +1847,7 @@ void btrfs_put_root(struct btrfs_root *root)
 		if (root->anon_dev)
 			free_anon_bdev(root->anon_dev);
 		free_root_extent_buffers(root);
+		btrfs_qgroup_clean_swapped_blocks(root);
 #ifdef CONFIG_BTRFS_DEBUG
 		spin_lock(&root->fs_info->fs_roots_radix_lock);
 		list_del_init(&root->leak_list);
@@ -1862,6 +1863,25 @@ void btrfs_free_fs_roots(struct btrfs_fs_info *fs_info)
 	struct btrfs_root *gang[8];
 	int i;
 
+	/*
+	 * Some dirty roots may still be linked to dirty_cowonly_roots after
+	 * the transaction is aborted. Cleanup them here.
+	 */
+	while (!list_empty(&fs_info->dirty_cowonly_roots)) {
+		struct btrfs_root *root;
+
+		root = list_first_entry(&fs_info->dirty_cowonly_roots,
+					struct btrfs_root, dirty_list);
+		list_del_init(&root->dirty_list);
+		/*
+		 * The dirty_log_pages should never be populated for
+		 * cowonly trees.
+		 */
+		spin_lock(&root->dirty_log_pages.lock);
+		ASSERT(RB_EMPTY_ROOT(&root->dirty_log_pages.state));
+		spin_unlock(&root->dirty_log_pages.lock);
+	}
+
 	while (!list_empty(&fs_info->dead_roots)) {
 		gang[0] = list_first_entry(&fs_info->dead_roots,
 					   struct btrfs_root, root_list);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`free_anon_bdev`
- 外部函数：`free_extent_buffer`
- 外部函数：`list_del_init`
- 外部函数：`list_empty`
- 外部函数：`list_first_entry`
- 外部函数：`spin_lock`
- 大写宏：`CONFIG_BTRFS_DEBUG`
- 大写宏：`NULL`
- 外部类型：`btrfs_root`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-a116a8aaeb` → 本草稿移入 `cases/defect/auto-linux-a116a8aaeb/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
