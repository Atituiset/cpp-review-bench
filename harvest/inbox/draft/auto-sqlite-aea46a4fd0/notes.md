# auto-sqlite-aea46a4fd0

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#581417a9b6064434b68c0136fb7b66457d9d8721](https://github.com/sqlite/sqlite/commit/581417a9b6064434b68c0136fb7b66457d9d8721) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-25 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 54 |
| 编译错误数（gcc syntax-only） | 12（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #581417a9b6064434b68c0136fb7b66457d9d8721 (https://github.com/sqlite/sqlite/commit/581417a9b6064434b68c0136fb7b66457d9d8721)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 3552；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 581417a9b6064434b68c0136fb7b66457d9d8721 Ensure that real values that can be stored as integers in 6 bytes or less are stored that way in indexes and WITHOUT ROW :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3527,14 +3527,19 @@ case OP_TypeCheck: {
   goto abort_due_to_error;
 }
 
-/* Opcode: Affinity P1 P2 * P4 *
+/* Opcode: Affinity P1 P2 * P4 P5
 ** Synopsis: affinity(r[P1@P2])
 **
 ** Apply affinities to a range of P2 registers starting with P1.
 **
 ** P4 is a string that is P2 characters long. The N-th character of the
 ** string indicates the column affinity that should be used for the N-th
 ** memory cell in the range.
+**
+** For REAL affinity, if P5 is non-zero and the value is an integer that fits
+** in 6 bytes (and so can be converted to floating point format and back
+** losslessly), mark the value as MEM_IntReal. Otherwise, if P5 is zero or
+** the integer value is too large, convert the value to a Mem_Real value. 
 */
 case OP_Affinity: {
   const char *zAffinity;   /* The affinity to be applied */
@@ -3549,9 +3554,16 @@ case OP_Affinity: {
     assert( zAffinity[0]==SQLITE_AFF_NONE || memIsValid(pIn1) );
     applyAffinity(pIn1, zAffinity[0], encoding);
     if( zAffinity[0]==SQLITE_AFF_REAL && (pIn1->flags & MEM_Int)!=0 ){
-      pIn1->u.r = (double)pIn1->u.i;
-      pIn1->flags |= MEM_Real;
-      pIn1->flags &= ~(MEM_Int|MEM_Str);
+      if( pOp->p5
+       && pIn1->u.i<=140737488355327LL && pIn1->u.i>=-140737488355328LL
+      ){
+        pIn1->flags |= MEM_IntReal;
+        pIn1->flags &= ~MEM_Int;
+      }else{
+        pIn1->u.r = (double)pIn1->u.i;
+        pIn1->flags |= MEM_Real;
+        pIn1->flags &= ~(MEM_Int|MEM_Str);
+      }
     }
     REGISTER_TRACE((int)(pIn1-aMem), pIn1);
     zAffinity++;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`affinity`
- 外部函数：`assert`
- 外部函数：`memIsValid`
- 外部函数：`printf`
- 外部函数：`sqlite3Atoi64`
- 外部函数：`sqlite3MemRealValueRC`
- 外部函数：`sqlite3RealSameAsInt`
- 外部函数：`sqlite3RealToI64`
- 外部函数：`sqlite3StrAccumFinish`
- 外部函数：`sqlite3StrAccumInit`
- 外部函数：`sqlite3VdbeCheckMemInvariants`
- 外部函数：`sqlite3VdbeIntegerAffinity`
- 外部函数：`sqlite3VdbeMemIsRowSet`
- 外部函数：`sqlite3VdbeMemStringify`
- 外部函数：`sqlite3_str_appendchar`
- 外部函数：`sqlite3_str_appendf`
- 外部函数：`testcase`
- 大写宏：`CPU`
- 大写宏：`FALSE`
- 大写宏：`NULL`
- 大写宏：`NUMERIC`
- 大写宏：`OPTIMIZATION`
- 大写宏：`SQLITE_AFF_FLEXNUM`
- 大写宏：`SQLITE_AFF_INTEGER`
- 大写宏：`SQLITE_AFF_NONE`
- 大写宏：`SQLITE_AFF_NUMERIC`
- 大写宏：`SQLITE_AFF_REAL`
- 大写宏：`SQLITE_AFF_TEXT`
- 大写宏：`SQLITE_OMIT_FLOATING_POINT`
- 大写宏：`TEXT`
- 外部类型：`Affinity`
- 外部类型：`Apply`
- 外部类型：`Hence`
- 外部类型：`It`
- 外部类型：`MEM_Blob`
- 外部类型：`MEM_Dyn`
- 外部类型：`MEM_Ephem`
- 外部类型：`MEM_Int`
- 外部类型：`MEM_IntReal`
- 外部类型：`MEM_Null`
- 外部类型：`MEM_Real`
- 外部类型：`MEM_Static`
- 外部类型：`MEM_Str`
- 外部类型：`MEM_Subtype`
- 外部类型：`MEM_Term`
- 外部类型：`MEM_Undefined`
- 外部类型：`MEM_Zero`
- 外部类型：`Mem`
- 外部类型：`OP_Affinity`
- 外部类型：`Only`
- 外部类型：`Opcode`
- 外部类型：`SQLITE_VdbeTrace`
- 外部类型：`StrAccum`
- 外部类型：`Synopsis`
- 外部类型：`The`
- 外部类型：`Ticket`
- 外部类型：`Use`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-aea46a4fd0` → 本草稿移入 `cases/defect/auto-sqlite-aea46a4fd0/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
