# auto-linux-9eb37c04cf

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#87aecd24f58e15256870befed777bbffc5d4a9d5](https://github.com/torvalds/linux/commit/87aecd24f58e15256870befed777bbffc5d4a9d5) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-16 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 27 |
| 编译错误数（gcc syntax-only） | 27（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #87aecd24f58e15256870befed777bbffc5d4a9d5 (https://github.com/torvalds/linux/commit/87aecd24f58e15256870befed777bbffc5d4a9d5)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 1228；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 87aecd24f58e15256870befed777bbffc5d4a9d5 Merge tag 'for-7.4/dm-fixes' of git://git.kernel.org/pub/scm/linux/kernel/git/device-mapper/linux-dm :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1225,10 +1225,13 @@ static int __create_snap(struct dm_pool_metadata *pmd,
 	__close_device(td);
 
 	if (r)
-		goto bad;
+		goto bad2;
 
 	return 0;
 
+bad2:
+	list_del(&td->list);
+	kfree(td);
 bad:
 	dm_btree_remove(&pmd->tl_info, pmd->root, &key, &pmd->root);
 	dm_btree_remove(&pmd->details_info, pmd->details_root,
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`dm_btree_remove`
- 外部函数：`previous`
- 外部函数：`service`
- 外部类型：`All`
- 外部类型：`Describes`
- 外部类型：`First`
- 外部类型：`Just`
- 外部类型：`Non`
- 外部类型：`Pre`
- 外部类型：`Reading`
- 外部类型：`Second`
- 外部类型：`Set`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Two`
- 外部类型：`We`
- 外部类型：`block_device`
- 外部类型：`dm_block_manager`
- 外部类型：`dm_block_t`
- 外部类型：`dm_btree_info`
- 外部类型：`dm_space_map`
- 外部类型：`dm_transaction_manager`
- 外部类型：`hlist_node`
- 外部类型：`list_head`
- 外部类型：`rw_semaphore`
- 外部类型：`sector_t`
- 外部类型：`thin_dev_t`
- 外部类型：`uint32_t`
- 外部类型：`uint64_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-9eb37c04cf` → 本草稿移入 `cases/defect/auto-linux-9eb37c04cf/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
