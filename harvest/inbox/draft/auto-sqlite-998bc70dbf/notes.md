# auto-sqlite-998bc70dbf

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#d785a1234e632131ea4f5643eb148a3824747abb](https://github.com/sqlite/sqlite/commit/d785a1234e632131ea4f5643eb148a3824747abb) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-12 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 2 |
| 编译错误数（gcc syntax-only） | 5（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #d785a1234e632131ea4f5643eb148a3824747abb (https://github.com/sqlite/sqlite/commit/d785a1234e632131ea4f5643eb148a3824747abb)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT d785a1234e632131ea4f5643eb148a3824747abb Ensure a statement that uses REPLACE conflict handling uses a statement journal if a later index is on an expression or  :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2460,6 +2460,14 @@ void sqlite3GenerateConstraintChecks(
     VdbeNoopComment((v, "prep index %s", pIdx->zName));
     iThisCur = iIdxCur+ix;
 
+    /* If an OE_Replace resolution earlier in the constraint checks may
+    ** have already deleted a conflicting row, and evaluating the partial
+    ** index WHERE clause or an indexed expression for this index might
+    ** raise an error, then a statement journal may be required to undo
+    ** the deletion.  */
+    if( seenReplace && (pIdx->pPartIdxWhere || pIdx->aColExpr) ){
+      sqlite3MultiWrite(pParse);
+    }
 
     /* Skip partial indices for which the WHERE clause is not true */
     if( pIdx->pPartIdxWhere ){
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 大写宏：`WHERE`
- 外部类型：`Skip`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-998bc70dbf` → 本草稿移入 `cases/defect/auto-sqlite-998bc70dbf/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
