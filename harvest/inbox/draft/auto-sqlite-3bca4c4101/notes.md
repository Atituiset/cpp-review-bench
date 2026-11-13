# auto-sqlite-3bca4c4101

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#00658c7f7ad08fa9c0e55f3cf3bb02bb5de85ec4](https://github.com/sqlite/sqlite/commit/00658c7f7ad08fa9c0e55f3cf3bb02bb5de85ec4) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-13 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 145 |
| 编译错误数（gcc syntax-only） | 81（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #00658c7f7ad08fa9c0e55f3cf3bb02bb5de85ec4 (https://github.com/sqlite/sqlite/commit/00658c7f7ad08fa9c0e55f3cf3bb02bb5de85ec4)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 1860；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 00658c7f7ad08fa9c0e55f3cf3bb02bb5de85ec4 When processing a "DEFAULT CAST(const AS type)" clause, apply the cast to "type" before the affinity of the column, not  :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1857,7 +1857,7 @@ static int valueFromExpr(
     u8 aff;
     assert( !ExprHasProperty(pExpr, EP_IntValue) );
     aff = sqlite3AffinityType(pExpr->u.zToken,0);
-    rc = valueFromExpr(db, pExpr->pLeft, enc, aff, ppVal, pCtx);
+    rc = valueFromExpr(db, pExpr->pLeft, enc, SQLITE_AFF_BLOB, ppVal, pCtx);
     testcase( rc!=SQLITE_OK );
     if( *ppVal ){
 #ifdef SQLITE_ENABLE_STAT4
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`defined`
- 外部函数：`memcmp`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`sqlite3AffinityType`
- 外部函数：`sqlite3AtoF`
- 外部函数：`sqlite3Atoi64`
- 外部函数：`sqlite3DbFree`
- 外部函数：`sqlite3DbFreeNN`
- 外部函数：`sqlite3DbMallocRaw`
- 外部函数：`sqlite3DbMallocSize`
- 外部函数：`sqlite3DbMallocZero`
- 外部函数：`sqlite3DbReallocOrFree`
- 外部函数：`sqlite3DbStrNDup`
- 外部函数：`sqlite3DecOrHexToI64`
- 外部函数：`sqlite3ErrorToParser`
- 外部函数：`sqlite3HexToBlob`
- 外部函数：`sqlite3KeyInfoOfIndex`
- 外部函数：`sqlite3MPrintf`
- 外部函数：`sqlite3OomFault`
- 外部函数：`sqlite3Realloc`
- 外部函数：`sqlite3Strlen30`
- 外部函数：`sqlite3ValueApplyAffinity`
- 外部函数：`sqlite3VdbeMemHandleBom`
- 外部函数：`sqlite3VdbeMemTranslate`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_msize`
- 外部函数：`sqlite3_mutex_held`
- 外部函数：`sqlite3_result_pointer`
- 外部函数：`strlen`
- 外部函数：`testcase`
- 外部函数：`valueFromExpr`
- 外部函数：`void`
- 外部函数：`xDel`
- 外部函数：`xFinalize`
- 大写宏：`BLOB`
- 大写宏：`CORRUPT_DB`
- 大写宏：`DEFAULT`
- 大写宏：`EIGHT_BYTE_ALIGNMENT`
- 大写宏：`ENC`
- 大写宏：`ENC_KEEPBOM`
- 大写宏：`ENC_MASK`
- 大写宏：`IMP`
- 大写宏：`LARGEST_INT64`
- 大写宏：`MAX`
- 大写宏：`NEVER`
- 大写宏：`NOMEM`
- 大写宏：`NULL`
- 大写宏：`ROUND8`
- 大写宏：`SMALLEST_INT64`
- 大写宏：`SQL`
- 大写宏：`SQLITE_AFF_BLOB`
- 大写宏：`SQLITE_AFF_INTEGER`
- 大写宏：`SQLITE_AFF_NUMERIC`
- 大写宏：`SQLITE_AFF_REAL`
- 大写宏：`SQLITE_AFF_TEXT`
- 大写宏：`SQLITE_DYNAMIC`
- 大写宏：`SQLITE_ENABLE_STAT4`
- 大写宏：`SQLITE_ERROR`
- 大写宏：`SQLITE_LIMIT_LENGTH`
- 大写宏：`SQLITE_MAX_LENGTH`
- 大写宏：`SQLITE_NOINLINE`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_NOMEM_BKPT`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_OMIT_BLOB_LITERAL`
- 大写宏：`SQLITE_OMIT_FLOATING_POINT`
- 大写宏：`SQLITE_OMIT_UTF16`
- 大写宏：`SQLITE_STATIC`
- 大写宏：`SQLITE_TOOBIG`
- 大写宏：`SQLITE_TRANSIENT`
- 大写宏：`SQLITE_UTF16BE`
- 大写宏：`SQLITE_UTF16LE`
- 大写宏：`SQLITE_UTF8`
- 大写宏：`STAT4`
- 大写宏：`TK_BLOB`
- 大写宏：`TK_CAST`
- 大写宏：`TK_FLOAT`
- 大写宏：`TK_FUNCTION`
- 大写宏：`TK_INTEGER`
- 大写宏：`TK_NULL`
- 大写宏：`TK_REGISTER`
- 大写宏：`TK_SPAN`
- 大写宏：`TK_STRING`
- 大写宏：`TK_TRUEFALSE`
- 大写宏：`TK_UMINUS`
- 大写宏：`TK_UPLUS`
- 大写宏：`UNUSED_PARAMETER`
- 外部类型：`Adjust`
- 外部类型：`Affinity`
- 外部类型：`An`
- 外部类型：`And`
- 外部类型：`BLOBs`
- 外部类型：`Blindly`
- 外部类型：`Bytes`
- 外部类型：`Cannot`
- 外部类型：`Compressed`
- 外部类型：`Counter`
- 外部类型：`Destructor`
- 外部类型：`EP_IntValue`
- 外部类型：`EP_TokenOnly`
- 外部类型：`Encoding`
- 外部类型：`Ex`
- 外部类型：`Except`
- 外部类型：`Expr`
- 外部类型：`FuncDef`
- 外部类型：`Handle`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Index`
- 外部类型：`It`
- 外部类型：`Length`
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
- 外部类型：`MEM_TypeMask`
- 外部类型：`MEM_Undefined`
- 外部类型：`MEM_Zero`
- 外部类型：`Maximum`
- 外部类型：`Mem`
- 外部类型：`Memory`
- 外部类型：`NULLs`
- 外部类型：`New`
- 外部类型：`No`
- 外部类型：`Number`
- 外部类型：`Only`
- 外部类型：`Parse`
- 外部类型：`RCStr`
- 外部类型：`Really`
- 外部类型：`Second`
- 外部类型：`String`
- 外部类型：`Text`
- 外部类型：`That`
- 外部类型：`The`
- 外部类型：`There`
- 外部类型：`This`
- 外部类型：`UnpackedRecord`
- 外部类型：`Value`
- 外部类型：`ValueNewStat4Ctx`
- 外部类型：`Write`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-3bca4c4101` → 本草稿移入 `cases/defect/auto-sqlite-3bca4c4101/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
