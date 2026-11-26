# auto-linux-afe4124f0e

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#40212b29a6f718d4e7fb3275cede181cb8e137cf](https://github.com/torvalds/linux/commit/40212b29a6f718d4e7fb3275cede181cb8e137cf) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-26 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 21 |
| 编译错误数（gcc syntax-only） | 52（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #40212b29a6f718d4e7fb3275cede181cb8e137cf (https://github.com/torvalds/linux/commit/40212b29a6f718d4e7fb3275cede181cb8e137cf)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 1056；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 40212b29a6f718d4e7fb3275cede181cb8e137cf Merge tag 'for-7.4-rc4-tag' of git://git.kernel.org/pub/scm/linux/kernel/git/kdave/linux :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1053,7 +1053,7 @@ static int __btrfs_update_delayed_inode(struct btrfs_trans_handle *trans,
 		ret = btrfs_search_slot(trans, root, &key, path, -1, 1);
 		if (unlikely(ret < 0)) {
 			btrfs_abort_transaction(trans, ret);
-			goto err_out;
+			goto out;
 		}
 		ASSERT(ret > 0);
 		ASSERT(path->slots[0] > 0);
@@ -1081,7 +1081,6 @@ static int __btrfs_update_delayed_inode(struct btrfs_trans_handle *trans,
 out:
 	btrfs_release_delayed_iref(node);
 	btrfs_release_path(path);
-err_out:
 	btrfs_delayed_inode_release_metadata(fs_info, node, (ret < 0));
 	btrfs_release_delayed_inode(node);
 	return ret;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`atomic_dec_return`
- 外部函数：`atomic_inc_return`
- 外部函数：`btrfs_abort_transaction`
- 外部函数：`btrfs_block_rsv_release`
- 外部函数：`btrfs_qgroup_convert_reserved_meta`
- 外部函数：`btrfs_qgroup_free_meta_prealloc`
- 外部函数：`btrfs_release_path`
- 外部函数：`btrfs_search_slot`
- 外部函数：`clear_bit`
- 外部函数：`cond_wake_up_nomb`
- 外部函数：`test_and_clear_bit`
- 外部函数：`test_bit`
- 外部函数：`trace_btrfs_space_reservation`
- 外部函数：`unlikely`
- 大写宏：`ASSERT`
- 大写宏：`BTRFS_DELAYED_NODE_DEL_IREF`
- 大写宏：`BTRFS_DELAYED_NODE_INODE_DIRTY`
- 大写宏：`NULL`
- 外部类型：`btrfs_block_rsv`
- 外部类型：`btrfs_delayed_node`
- 外部类型：`btrfs_delayed_root`
- 外部类型：`btrfs_fs_info`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-afe4124f0e` → 本草稿移入 `cases/defect/auto-linux-afe4124f0e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
