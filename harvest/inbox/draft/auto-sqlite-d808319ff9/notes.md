# auto-sqlite-d808319ff9

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#17f128f7466f21396e05591067c36ad8dbbb0f3d](https://github.com/sqlite/sqlite/commit/17f128f7466f21396e05591067c36ad8dbbb0f3d) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-20 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 7 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #17f128f7466f21396e05591067c36ad8dbbb0f3d (https://github.com/sqlite/sqlite/commit/17f128f7466f21396e05591067c36ad8dbbb0f3d)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 17f128f7466f21396e05591067c36ad8dbbb0f3d Fix an adverse interaction between triggers changing a table that uses :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3978,6 +3978,7 @@ struct Parse {
   Table *pTriggerTab;  /* Table triggers are being coded for */
   TriggerPrg *pTriggerPrg;  /* Linked list of coded triggers */
   ParseCleanup *pCleanup;   /* List of cleanup operations to run after parse */
+  Trigger *pTrigCoded;      /* Trigger whose sub-program is being coded */
 
   /**************************************************************************
   ** Fields above must be initialized to zero.  The fields that follow,
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部类型：`Fields`
- 外部类型：`Linked`
- 外部类型：`List`
- 外部类型：`ParseCleanup`
- 外部类型：`Table`
- 外部类型：`The`
- 外部类型：`TriggerPrg`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-d808319ff9` → 本草稿移入 `cases/defect/auto-sqlite-d808319ff9/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
