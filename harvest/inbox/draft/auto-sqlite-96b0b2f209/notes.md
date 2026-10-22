# auto-sqlite-96b0b2f209

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#ac429ff80483f25c0f7d1ea02f3e21040ec388e3](https://github.com/sqlite/sqlite/commit/ac429ff80483f25c0f7d1ea02f3e21040ec388e3) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-22 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 11 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #ac429ff80483f25c0f7d1ea02f3e21040ec388e3 (https://github.com/sqlite/sqlite/commit/ac429ff80483f25c0f7d1ea02f3e21040ec388e3)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 824；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT ac429ff80483f25c0f7d1ea02f3e21040ec388e3 Index lookup of an integer column using value -9223372036854775808.0 :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -798,8 +798,28 @@ int sqlite3VdbeBooleanValue(Mem *pMem, int ifNull){
 /*
 ** The MEM structure is already a MEM_Real or MEM_IntReal. Try to 
 ** make it a MEM_Int if we can.
-*/
-void sqlite3VdbeIntegerAffinity(Mem *pMem){
+**
+** This routine only converts REAL values into INTEGER if the REAL
+** has no fractional part and if the value can be exactly expressed
+** as a signed 64-bit integer.
+**
+** The bFromText parameter is true if the MEM was originally text.
+** If the value was originally text then this routine will not convert
+** -9223372036854775808.0 into -9223372036854775808 because if the value
+** was originally text, it would have been converted into an integer
+** already, and since it was not, that means that the original text
+** content had the last four significant digits greater than "5808"
+** and the hence the text value was not really -9223372036854775808.0
+** but rather some nearby value that happens to have the same IEEE 754
+** representation. We don't want text '-9223372036854775900.0' to get
+** converted into -9223372036854775808.
+**
+** But if bFromText is false, then the value started out as REAL and
+** so it is ok to convert -9223372036854775808.0 into -9223372036854775808
+** in that case, because -9223372036854775808.0 is the exact value of
+** the underlying IEEE 754 representation.
+*/
+void sqlite3VdbeIntegerAffinity(Mem *pMem, int bFromText){
   assert( pMem!=0 );
   assert( pMem->flags & (MEM_Real|MEM_IntReal) );
   assert( !sqlite3VdbeMemIsRowSet(pMem) );
@@ -821,7 +841,9 @@ void sqlite3VdbeIntegerAffinity(Mem *pMem){
     ** the second condition under the assumption that addition overflow causes
     ** values to wrap around.
     */
-    if( pMem->u.r==ix && ix>SMALLEST_INT64 && ix<LARGEST_INT64 ){
+    if( pMem->u.r==ix && ix<LARGEST_INT64
+     && (!bFromText || ix>SMALLEST_INT64)
+    ){
       pMem->u.i = ix;
       MemSetTypeFlag(pMem, MEM_Int);
     }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 大写宏：`LARGEST_INT64`
- 大写宏：`MEM`
- 大写宏：`SMALLEST_INT64`
- 外部类型：`MEM_Blob`
- 外部类型：`MEM_Dyn`
- 外部类型：`MEM_Int`
- 外部类型：`MEM_IntReal`
- 外部类型：`MEM_Real`
- 外部类型：`Mem`
- 外部类型：`The`
- 外部类型：`Try`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-96b0b2f209` → 本草稿移入 `cases/defect/auto-sqlite-96b0b2f209/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
