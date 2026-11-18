# auto-linux-ab6b09660e

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#63fdb511c27afcebce797e3db580b3534ddfa639](https://github.com/torvalds/linux/commit/63fdb511c27afcebce797e3db580b3534ddfa639) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-18 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 11 |
| 编译错误数（gcc syntax-only） | 17（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #63fdb511c27afcebce797e3db580b3534ddfa639 (https://github.com/torvalds/linux/commit/63fdb511c27afcebce797e3db580b3534ddfa639)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 17（原始 PR diff 行 1111；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 63fdb511c27afcebce797e3db580b3534ddfa639 Merge tag 'mm-core-7.4-rc4' of git://git.kernel.org/pub/scm/linux/kernel/git/mm/core :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1108,7 +1108,9 @@ static int fuse_ref_folio(struct fuse_copy_state *cs, struct folio *folio,
 	if (cs->nr_segs >= cs->pipe->max_usage)
 		return -EIO;
 
-	folio_get(folio);
+	if (!folio_get_checked(folio))
+		return -EFAULT;
+
 	err = unlock_request(cs->req);
 	if (err) {
 		folio_put(folio);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`clear_bit`
- 外部函数：`folio_get`
- 外部函数：`folio_put`
- 外部函数：`spin_lock`
- 外部函数：`spin_unlock`
- 外部函数：`test_bit`
- 大写宏：`EIO`
- 大写宏：`ENOENT`
- 大写宏：`FR_ABORTED`
- 大写宏：`FR_LOCKED`
- 外部类型：`fuse_req`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-ab6b09660e` → 本草稿移入 `cases/defect/auto-linux-ab6b09660e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
