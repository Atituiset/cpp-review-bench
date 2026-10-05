# auto-sqlite-476e35e9fb

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#5e54ba5160b572c00539b1b19c05df590701a7df](https://github.com/sqlite/sqlite/commit/5e54ba5160b572c00539b1b19c05df590701a7df) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-01 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 12 |
| 编译错误数（gcc syntax-only） | 5（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #5e54ba5160b572c00539b1b19c05df590701a7df (https://github.com/sqlite/sqlite/commit/5e54ba5160b572c00539b1b19c05df590701a7df)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 64；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 5e54ba5160b572c00539b1b19c05df590701a7df Fix a harmless compiler warning. :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -61,8 +61,7 @@ static void updateVirtualTable(
 static SQLITE_NOINLINE void columnDefaultUncommonCase(
   Vdbe *v,          /* Byte code under construction */
   Table *pTab,      /* The table */
-  Column *pCol,     /* Which column of the table */
-  int iReg          /* Register in which results are stored */
+  Column *pCol      /* Which column of the table */
 ){
   sqlite3_value *pValue = 0;
   u8 enc = ENC(sqlite3VdbeDb(v));
@@ -86,7 +85,7 @@ void sqlite3ColumnDefault(
   assert( pTab->nCol>i );
   pCol = &pTab->aCol[i];
   if( pCol->iDflt ){
-    columnDefaultUncommonCase(v,pTab,pCol,iReg);
+    columnDefaultUncommonCase(v,pTab,pCol);
   }
 #ifndef SQLITE_OMIT_FLOATING_POINT
   if( pCol->affinity==SQLITE_AFF_REAL ){
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`sqlite3VdbeDb`
- 大写宏：`ENC`
- 大写宏：`SQLITE_AFF_REAL`
- 大写宏：`SQLITE_NOINLINE`
- 大写宏：`SQLITE_OMIT_FLOATING_POINT`
- 外部类型：`Byte`
- 外部类型：`Column`
- 外部类型：`Register`
- 外部类型：`Table`
- 外部类型：`The`
- 外部类型：`Vdbe`
- 外部类型：`Which`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-476e35e9fb` → 本草稿移入 `cases/defect/auto-sqlite-476e35e9fb/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
