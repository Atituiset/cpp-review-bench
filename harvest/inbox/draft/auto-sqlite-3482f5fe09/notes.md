# auto-sqlite-3482f5fe09

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#7fd436eaf965da688c22babb5b5042149ff12947](https://github.com/sqlite/sqlite/commit/7fd436eaf965da688c22babb5b5042149ff12947) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-17 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 66 |
| 编译错误数（gcc syntax-only） | 19（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #7fd436eaf965da688c22babb5b5042149ff12947 (https://github.com/sqlite/sqlite/commit/7fd436eaf965da688c22babb5b5042149ff12947)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 7fd436eaf965da688c22babb5b5042149ff12947 Fix a problem with reading large UTF16 strings with a BOM out of the :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1264,11 +1264,16 @@ void sqlite3VdbeMemMove(Mem *pTo, Mem *pFrom){
 ** either case, SQLITE_TOOBIG is returned.
 **
 ** The "enc" parameter is the text encoding for the string, or zero
-** to store a blob.
+** to store a blob.  If enc not 0 or SQLITE_UTF8 and the string
+** begins with a BOM, the BOM is removed.  To retain the BOM on a UTF16
+** string, use a non-negative "n" and set enc to SQLITE_UTF8, and
+** separately set pMem->enc to the correct encoding after this function
+** returns.
 **
-** If n is negative, then the string consists of all bytes up to but
-** excluding the first zero character.  The n parameter must be
-** non-negative for blobs.
+** If n is negative and enc is not zero and is set correctly, then the
+** string consists of all bytes up to but excluding the first zero
+** character.  The n parameter must be non-negative for blobs or if
+** enc is not set accurately.
 */
 int sqlite3VdbeMemSetStr(
   Mem *pMem,          /* Memory cell to set to string value */
@@ -1357,6 +1362,10 @@ int sqlite3VdbeMemSetStr(
   pMem->enc = enc;
 
 #ifndef SQLITE_OMIT_UTF16
+  /* tag-20261016-1:  To retain the BOM, use enc==SQLITE_UTF8, even
+  ** for UTF-16 text, and then set pMem->enc correctly after this
+  ** function returns.  Note, however, that n must be non-negative
+  ** for this trick to work. */
   if( enc>SQLITE_UTF8 && sqlite3VdbeMemHandleBom(pMem) ){
     return SQLITE_NOMEM_BKPT;
   }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`sqlite3DbFreeNN`
- 外部函数：`sqlite3DbMallocRaw`
- 外部函数：`sqlite3DbMallocSize`
- 外部函数：`sqlite3DbReallocOrFree`
- 外部函数：`sqlite3Realloc`
- 外部函数：`sqlite3VdbeMemIsRowSet`
- 外部函数：`sqlite3VdbeMemSetNull`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_mutex_held`
- 外部函数：`sqlite3_result_pointer`
- 外部函数：`testcase`
- 外部函数：`xDel`
- 外部函数：`xFinalize`
- 大写宏：`CORRUPT_DB`
- 大写宏：`ENC`
- 大写宏：`IMP`
- 大写宏：`NULL`
- 大写宏：`SQLITE_DYNAMIC`
- 大写宏：`SQLITE_NOINLINE`
- 大写宏：`SQLITE_NOMEM_BKPT`
- 大写宏：`SQLITE_OK`
- 外部类型：`An`
- 外部类型：`Cannot`
- 外部类型：`FuncDef`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`MEM_Agg`
- 外部类型：`MEM_Blob`
- 外部类型：`MEM_Cleared`
- 外部类型：`MEM_Dyn`
- 外部类型：`MEM_Ephem`
- 外部类型：`MEM_FromBind`
- 外部类型：`MEM_Int`
- 外部类型：`MEM_IntReal`
- 外部类型：`MEM_Null`
- 外部类型：`MEM_Real`
- 外部类型：`MEM_Static`
- 外部类型：`MEM_Str`
- 外部类型：`MEM_Subtype`
- 外部类型：`MEM_Term`
- 外部类型：`MEM_Undefined`
- 外部类型：`Mem`
- 外部类型：`Memory`
- 外部类型：`NULLs`
- 外部类型：`No`
- 外部类型：`That`
- 外部类型：`The`
- 外部类型：`There`
- 外部类型：`This`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-3482f5fe09` → 本草稿移入 `cases/defect/auto-sqlite-3482f5fe09/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
