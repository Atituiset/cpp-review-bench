# auto-sqlite-5fd852a81f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#d85a80711cfef59a0742362a9b2ee5c70f5b8267](https://github.com/sqlite/sqlite/commit/d85a80711cfef59a0742362a9b2ee5c70f5b8267) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-11 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 144 |
| 编译错误数（gcc syntax-only） | 38（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #d85a80711cfef59a0742362a9b2ee5c70f5b8267 (https://github.com/sqlite/sqlite/commit/d85a80711cfef59a0742362a9b2ee5c70f5b8267)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 8（原始 PR diff 行 2701；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT d85a80711cfef59a0742362a9b2ee5c70f5b8267 Back out a minor performance optimization associated with :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -41,6 +41,7 @@ Vdbe *sqlite3VdbeCreate(Parse *pParse){
   assert( pParse->aLabel==0 );
   assert( pParse->nLabel==0 );
   assert( p->nOpAlloc==0 );
+  assert( pParse->szOpAlloc==0 );
   sqlite3VdbeAddOp2(p, OP_Init, 0, 1);
   return p;
 }
@@ -191,7 +192,8 @@ static int growOpArray(Vdbe *v, int nOp){
   assert( nNew>=(v->nOpAlloc+nOp) );
   pNew = sqlite3DbRealloc(p->db, v->aOp, nNew*sizeof(Op));
   if( pNew ){
-    v->nOpAlloc = sqlite3DbMallocSize(p->db, pNew)/sizeof(Op);
+    p->szOpAlloc = sqlite3DbMallocSize(p->db, pNew);
+    v->nOpAlloc = p->szOpAlloc/sizeof(Op);
     v->aOp = pNew;
   }
   return (pNew ? SQLITE_OK : SQLITE_NOMEM_BKPT);
@@ -2698,7 +2700,7 @@ void sqlite3VdbeMakeReady(
   n = ROUND8P(sizeof(Op)*p->nOp);             /* Bytes of opcode memory used */
   x.pSpace = &((u8*)p->aOp)[n];               /* Unused opcode memory */
   assert( EIGHT_BYTE_ALIGNMENT(x.pSpace) );
-  x.nFree = ROUNDDOWN8((p->nOpAlloc-p->nOp)*sizeof(Op)); /* Bytes unused mem */
+  x.nFree = ROUNDDOWN8(pParse->szOpAlloc - n);  /* Bytes unused memory */
   assert( x.nFree>=0 );
   assert( EIGHT_BYTE_ALIGNMENT(&x.pSpace[x.nFree]) );
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`abort`
- 外部函数：`assert`
- 外部函数：`defined`
- 外部函数：`fflush`
- 外部函数：`fprintf`
- 外部函数：`k`
- 外部函数：`memcpy`
- 外部函数：`operation`
- 外部函数：`s`
- 外部函数：`sqlite3BeginBenignMalloc`
- 外部函数：`sqlite3DbMallocSize`
- 外部函数：`sqlite3DbRealloc`
- 外部函数：`sqlite3EndBenignMalloc`
- 外部函数：`sqlite3OomFault`
- 外部函数：`sqlite3OpcodeName`
- 外部函数：`sqlite3StrAccumFinish`
- 外部函数：`sqlite3StrAccumInit`
- 外部函数：`sqlite3Strlen30`
- 外部函数：`sqlite3VdbeAddOp4Int`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_snprintf`
- 外部函数：`sqlite3_str_append`
- 外部函数：`sqlite3_str_appendall`
- 外部函数：`sqlite3_str_appendchar`
- 外部函数：`sqlite3_str_appendf`
- 外部函数：`strcmp`
- 外部函数：`strncmp`
- 大写宏：`ADD`
- 大写宏：`AND`
- 大写宏：`BINARY`
- 大写宏：`BITAND`
- 大写宏：`BITNOT`
- 大写宏：`BITOR`
- 大写宏：`CONCAT`
- 大写宏：`DBL13`
- 大写宏：`DIV`
- 大写宏：`EIGHT_BYTE_ALIGNMENT`
- 大写宏：`FILE`
- 大写宏：`HEX23`
- 大写宏：`INT13`
- 大写宏：`INT32_TO_64`
- 大写宏：`INTARRAY`
- 大写宏：`ISNOT`
- 大写宏：`ISNULL`
- 大写宏：`KEYINFO_ORDER_BIGNULL`
- 大写宏：`KEYINFO_ORDER_DESC`
- 大写宏：`LARGEST_UINT64`
- 大写宏：`LSHIFT`
- 大写宏：`MINUS`
- 大写宏：`MUL`
- 大写宏：`NOT`
- 大写宏：`NOTNULL`
- 大写宏：`NULL`
- 大写宏：`OOM`
- 大写宏：`P4_COLLSEQ`
- 大写宏：`P4_EXPR`
- 大写宏：`P4_FUNCCTX`
- 大写宏：`P4_FUNCDEF`
- 大写宏：`P4_INDEX`
- 大写宏：`P4_INT32`
- 大写宏：`P4_INTARRAY`
- 大写宏：`P4_KEYINFO`
- 大写宏：`P4_MEM`
- 大写宏：`P4_NOTUSED`
- 大写宏：`P4_SUBPROGRAM`
- 大写宏：`P4_SUBRTNSIG`
- 大写宏：`P4_TABLE`
- 大写宏：`P4_VTAB`
- 大写宏：`PLUS`
- 大写宏：`REM`
- 大写宏：`ROUND8P`
- 大写宏：`ROUNDDOWN8`
- 大写宏：`RSHIFT`
- 大写宏：`SQLITE_DEBUG`
- 大写宏：`SQLITE_ENABLE_COLUMN_USED_MASK`
- 大写宏：`SQLITE_ENABLE_CURSOR_HINTS`
- 大写宏：`SQLITE_ENABLE_EXPLAIN_COMMENTS`
- 大写宏：`SQLITE_ENABLE_STMT_SCANSTATUS`
- 大写宏：`SQLITE_LIMIT_VDBE_OP`
- 大写宏：`SQLITE_MAX_LENGTH`
- 大写宏：`SQLITE_NOINLINE`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_NOMEM_BKPT`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_OMIT_VIRTUALTABLE`
- 大写宏：`SQLITE_TEST_REALLOC_STRESS`
- 大写宏：`SQLITE_VDBE_COVERAGE`
- 大写宏：`SUB`
- 大写宏：`TK_AND`
- 大写宏：`TK_BITAND`
- 大写宏：`TK_BITNOT`
- 大写宏：`TK_BITOR`
- 大写宏：`TK_COLUMN`
- 大写宏：`TK_CONCAT`
- 大写宏：`TK_EQ`
- 大写宏：`TK_GE`
- 大写宏：`TK_GT`
- 大写宏：`TK_INTEGER`
- 大写宏：`TK_IS`
- 大写宏：`TK_ISNOT`
- 大写宏：`TK_ISNULL`
- 大写宏：`TK_LE`
- 大写宏：`TK_LSHIFT`
- 大写宏：`TK_LT`
- 大写宏：`TK_MINUS`
- 大写宏：`TK_NE`
- 大写宏：`TK_NOT`
- 大写宏：`TK_NOTNULL`
- 大写宏：`TK_NULL`
- 大写宏：`TK_OR`
- 大写宏：`TK_PLUS`
- 大写宏：`TK_REGISTER`
- 大写宏：`TK_REM`
- 大写宏：`TK_RSHIFT`
- 大写宏：`TK_SLASH`
- 大写宏：`TK_STAR`
- 大写宏：`TK_STRING`
- 大写宏：`TK_UMINUS`
- 大写宏：`TK_UPLUS`
- 大写宏：`UNUSED_PARAMETER`
- 大写宏：`VDBE`
- 大写宏：`VDBE_INIT_STATE`
- 大写宏：`VDBE_PROFILE`
- 外部类型：`Bytes`
- 外部类型：`CollSeq`
- 外部类型：`EP_IntValue`
- 外部类型：`Ensure`
- 外部类型：`Expr`
- 外部类型：`FuncDef`
- 外部类型：`KeyInfo`
- 外部类型：`MEM_Blob`
- 外部类型：`MEM_Int`
- 外部类型：`MEM_IntReal`
- 外部类型：`MEM_Null`
- 外部类型：`MEM_Real`
- 外部类型：`MEM_Str`
- 外部类型：`Mem`
- 外部类型：`Normal`
- 外部类型：`OP_Init`
- 外部类型：`Oom`
- 外部类型：`Op`
- 外部类型：`Optional`
- 外部类型：`Parse`
- 外部类型：`Previously`
- 外部类型：`Replicate`
- 外部类型：`SQLITE_VdbeAddopTrace`
- 外部类型：`StrAccum`
- 外部类型：`SubrtnSig`
- 外部类型：`The`
- 外部类型：`Unused`
- 外部类型：`Vdbe`
- 外部类型：`VdbeOp`
- 外部类型：`With`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-5fd852a81f` → 本草稿移入 `cases/defect/auto-sqlite-5fd852a81f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
