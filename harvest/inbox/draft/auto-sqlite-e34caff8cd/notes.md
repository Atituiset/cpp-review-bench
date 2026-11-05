# auto-sqlite-e34caff8cd

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#f4ebb9943ff6fe8a69f82f1acc59f409be387fbb](https://github.com/sqlite/sqlite/commit/f4ebb9943ff6fe8a69f82f1acc59f409be387fbb) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-05 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 9 |
| 编译错误数（gcc syntax-only） | 5（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #f4ebb9943ff6fe8a69f82f1acc59f409be387fbb (https://github.com/sqlite/sqlite/commit/f4ebb9943ff6fe8a69f82f1acc59f409be387fbb)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT f4ebb9943ff6fe8a69f82f1acc59f409be387fbb Fix a problem with ALTER TABLE in cases where an identifier in a view or trigger's ORDER BY clause matches a column bein :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3286,6 +3286,7 @@ struct ExprList {
       unsigned bNulls :1;     /* True if explicit "NULLS FIRST/LAST" */
       unsigned bUsed :1;      /* This column used in a SF_NestedFrom subquery */
       unsigned bUsingTerm:1;  /* Term from the USING clause of a NestedFrom */
+      unsigned bStarName:1;   /* RENAME only: zEName from expanding a "*" */
       unsigned bNoExpand: 1;  /* Term is an auxiliary in NestedFrom and should
                               ** not be expanded by "*" in parent queries */
     } fg;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 大写宏：`FIRST`
- 大写宏：`LAST`
- 大写宏：`NULLS`
- 大写宏：`USING`
- 外部类型：`NestedFrom`
- 外部类型：`SF_NestedFrom`
- 外部类型：`Term`
- 外部类型：`This`
- 外部类型：`True`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-e34caff8cd` → 本草稿移入 `cases/defect/auto-sqlite-e34caff8cd/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
