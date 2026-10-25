# auto-linux-0203f6ccaf

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#21a7f44fd0765fcc5194130b28767786eba1d1e7](https://github.com/torvalds/linux/commit/21a7f44fd0765fcc5194130b28767786eba1d1e7) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-25 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 37 |
| 编译错误数（gcc syntax-only） | 25（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #21a7f44fd0765fcc5194130b28767786eba1d1e7 (https://github.com/torvalds/linux/commit/21a7f44fd0765fcc5194130b28767786eba1d1e7)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1293；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 21a7f44fd0765fcc5194130b28767786eba1d1e7 Merge tag 'devicetree-for-7.4' of git://git.kernel.org/pub/scm/linux/kernel/git/robh/linux :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1282,26 +1282,3 @@ int of_overlay_remove(int *ovcs_id)
 	return ret;
 }
 EXPORT_SYMBOL_GPL(of_overlay_remove);
-
-/**
- * of_overlay_remove_all() - Reverts and frees all overlay changesets
- *
- * Removes all overlays from the system in the correct order.
- *
- * Return: 0 on success, or a negative error number
- */
-int of_overlay_remove_all(void)
-{
-	struct overlay_changeset *ovcs, *ovcs_n;
-	int ret;
-
-	/* the tail of list is guaranteed to be safe to remove */
-	list_for_each_entry_safe_reverse(ovcs, ovcs_n, &ovcs_list, ovcs_list) {
-		ret = of_overlay_remove(&ovcs->id);
-		if (ret)
-			return ret;
-	}
-
-	return 0;
-}
-EXPORT_SYMBOL_GPL(of_overlay_remove_all);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__of_changeset_revert_entries`
- 外部函数：`__of_changeset_revert_notify`
- 外部函数：`blocking_notifier_call_chain`
- 外部函数：`idr_find`
- 外部函数：`idr_remove`
- 外部函数：`kfree`
- 外部函数：`list_del`
- 外部函数：`mutex_lock`
- 外部函数：`mutex_unlock`
- 外部函数：`notifier_to_errno`
- 外部函数：`of_changeset_destroy`
- 外部函数：`of_node_put`
- 外部函数：`of_overlay_action_name`
- 外部函数：`pr_debug`
- 外部函数：`pr_err`
- 外部函数：`s`
- 大写宏：`EBUSY`
- 大写宏：`ENODEV`
- 大写宏：`EXPORT_SYMBOL_GPL`
- 大写宏：`OF_OVERLAY_INIT`
- 大写宏：`OF_OVERLAY_POST_REMOVE`
- 大写宏：`OF_OVERLAY_PRE_APPLY`
- 大写宏：`OF_OVERLAY_PRE_REMOVE`
- 外部类型：`Could`
- 外部类型：`If`
- 外部类型：`Note`
- 外部类型：`Removes`
- 外部类型：`Return`
- 外部类型：`Reverts`
- 外部类型：`There`
- 外部类型：`This`
- 外部类型：`device_node`
- 外部类型：`list_head`
- 外部类型：`of_changeset`
- 外部类型：`of_changeset_entry`
- 外部类型：`of_overlay_notify_action`
- 外部类型：`of_overlay_notify_data`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-0203f6ccaf` → 本草稿移入 `cases/defect/auto-linux-0203f6ccaf/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
