# auto-sqlite-f134f5b6f8

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#6e55a61a68cf58544ef294f218be65fb4310c654](https://github.com/sqlite/sqlite/commit/6e55a61a68cf58544ef294f218be65fb4310c654) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-10 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 79 |
| 编译错误数（gcc syntax-only） | 4（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #6e55a61a68cf58544ef294f218be65fb4310c654 (https://github.com/sqlite/sqlite/commit/6e55a61a68cf58544ef294f218be65fb4310c654)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 7149；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 6e55a61a68cf58544ef294f218be65fb4310c654 Fix some problems with handling corrupt meta-value arrays in vec1. :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -668,7 +668,7 @@ static void vec1HeapInsert(Vec1AnnHeap *p, sqlite3_int64 iRowid, double fDist){
 ** encountered.
 */
 static int vec1BufferGrow(Vec1Buffer *pBuf, sqlite3_int64 nByte){
-  if( pBuf->n+nByte>pBuf->nAlloc ){
+  if( nByte>pBuf->nAlloc-pBuf->n ){
     sqlite3_int64 nNew = pBuf->nAlloc;
     unsigned char *aNew = 0;
 
@@ -5421,6 +5421,11 @@ static void vec1ModelFreeTCent(Vec1Model *pMod){
 #define VEC1_META_4BYTEMAX   +2147483646
 #define VEC1_META_4BYTENULL  +2147483647
 
+/* When a meta-value array is loaded from disk, at least VEC1_META_PADDING
+** extra bytes of memory are allocated. This is to allow readers to safely
+** overread the record slightly if it is corrupt.  */
+#define VEC1_META_PADDING 20
+
 typedef struct Vec1Tab Vec1Tab;
 typedef struct Vec1Csr Vec1Csr;
 
@@ -7020,13 +7025,21 @@ static int vec1PutVarint(u8 *aBuf, u64 v){
 }
 
 static int vec1GetVarint(const u8 *aBuf, u64 *piVal){
-  int nRet = 0;
   u64 out = 0;
-  do {
-    out = (out<<7) + (aBuf[nRet] & 0x7F);
-  } while( aBuf[nRet++] & 0x80 );
+  int ii = 0;
+
+  out = (aBuf[0] & 0x7F);
+  if( aBuf[0] & 0x80 ){
+    for(ii=1; ii<9; ii++){
+      out = (out<<7) + (aBuf[ii] & 0x7F);
+      if( (aBuf[ii] & 0x80)==0 ){
+        break;
+      }
+    }
+  }
+
   *piVal = out;
-  return nRet;
+  return ii+1;
 }
 
 /*
@@ -7122,6 +7135,7 @@ static void vec1BinaryQuantLUTBuild(
 */
 static int vec1ReadMeta(
   Vec1Tab *pTab,                  /* Vec1 virtual table */
+  u32 nExpect,                    /* Expected number of entries */
   Vec1Buffer *pBuf,               /* Buffer to read into */
   i64 iId,                        /* Id of %_idx row */
   int iMeta                       /* Index of meta-column */
@@ -7137,18 +7151,47 @@ static int vec1ReadMeta(
     sqlite3_bind_int64(pStmt, 1, iMetaId);
     if( SQLITE_ROW==sqlite3_step(pStmt) ){
       int nByte = sqlite3_column_bytes(pStmt, 0);
-      rc = vec1BufferGrow(pBuf, nByte);
+      rc = vec1BufferGrow(pBuf, nByte + VEC1_META_PADDING);
       if( rc==SQLITE_OK ){
         const u8 *a = sqlite3_column_blob(pStmt, 0);
         memcpy(pBuf->a, a, nByte);
+        memset(&pBuf->a[nByte], 0, VEC1_META_PADDING);
         pBuf->n = nByte;
       }
     }
-
     vec1StmtReset(&rc, pStmt);
-    if( rc==SQLITE_OK && pBuf->n==0 ){
-      rc = VEC1_CORRUPT;
+
+    /* Check, so far as is possible, that the meta-value blob is the 
+    ** expected size. It is not possible to check if the meta-value blob
+    ** uses the GENERIC format, due to the variable length fields. Code
+    ** that handles GENERIC meta-value blobs has to dynamically verify
+    ** the buffer size. */
+    if( rc==SQLITE_OK ){
+      if( pBuf->n<VEC1_META_SZHDR ){
+        rc = VEC1_CORRUPT;
+      }else{
+        u32 tflag = (vec1GetU32(&pBuf->a[0]) & VEC1_META_TYPEMASK);
+        u32 nEntry = vec1GetU32(&pBuf->a[4]);
+        i64 sz = 0;
+        if( nEntry!=nExpect ){
+          rc = VEC1_CORRUPT;
+        }
+        if( tflag==VEC1_META_1BYTEINT ){
+          sz = 1;
+        }else if( tflag==VEC1_META_4BYTEINT ){
+          sz = sizeof_u32;
+        }else if( tflag==VEC1_META_REAL ){
+          sz = sizeof_f64;
+        }else if( tflag!=VEC1_META_GENERIC ){
+          rc = VEC1_CORRUPT;
+        }
+
+        if( sz && pBuf->n!=(VEC1_META_SZHDR + sz*nEntry) ){
+          rc = VEC1_CORRUPT;
+        }
+      }
     }
+
   }
 
   return rc;
@@ -7162,11 +7205,12 @@ static int vec1ReadMeta(
 **        4: Real value. 8 byte payload.
 */
 static int vec1MetaValueRead(
-  Vec1Buffer *pMeta, 
-  int *piOff, 
+  Vec1Buffer *pMeta,
+  int *piOff,
   Vec1MetaValue *pVal
 ){
-  int iOff = *piOff;
+  i64 iOff = (i64)*piOff;
+
   switch( pMeta->a[iOff] ){
     case 0: {
       pVal->eType = SQLITE_NULL;
@@ -7207,14 +7251,18 @@ static int vec1MetaValueRead(
       u64 eType = 0;
       iOff += vec1GetVarint(&pMeta->a[iOff], &eType);
       pVal->pPtr = &pMeta->a[iOff];
-      pVal->iVal = (eType-5) / 2;
-      iOff += (int)pVal->iVal;
+      pVal
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__volatile__`
- 外部函数：`defined`
- 外部函数：`length`
- 外部函数：`memcmp`
- 外部函数：`memcpy`
- 外部函数：`memmove`
- 外部函数：`memset`
- 外部函数：`min`
- 外部函数：`sqlite3_bind_int64`
- 外部函数：`sqlite3_column_blob`
- 外部函数：`sqlite3_column_bytes`
- 外部函数：`sqlite3_errmsg`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_malloc64`
- 外部函数：`sqlite3_prepare_v2`
- 外部函数：`sqlite3_reset`
- 外部函数：`sqlite3_step`
- 外部函数：`sqlite3_value_type`
- 外部函数：`sqlite3_vmprintf`
- 外部函数：`sum`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 外部函数：`vec1CorruptError`
- 外部函数：`vec1MetaValueCheck`
- 外部函数：`vec1WriterMetaFromPacked`
- 大写宏：`AND`
- 大写宏：`BETWEEN`
- 大写宏：`CONFLICT`
- 大写宏：`DELETE`
- 大写宏：`FROM`
- 大写宏：`INSERT`
- 大写宏：`INTO`
- 大写宏：`LIMIT`
- 大写宏：`NDEBUG`
- 大写宏：`NULL`
- 大写宏：`ORDER`
- 大写宏：`OUT`
- 大写宏：`REPLACE`
- 大写宏：`RETURNING`
- 大写宏：`SELECT`
- 大写宏：`SET`
- 大写宏：`SQLITE_BLOB`
- 大写宏：`SQLITE_CORRUPT_VTAB`
- 大写宏：`SQLITE_FLOAT`
- 大写宏：`SQLITE_INTEGER`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_NULL`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_ROW`
- 大写宏：`SQLITE_TEXT`
- 大写宏：`UPDATE`
- 大写宏：`VALUES`
- 大写宏：`WHERE`
- 外部类型：`All`
- 外部类型：`Allocate`
- 外部类型：`Bucket`
- 外部类型：`Buffer`
- 外部类型：`For`
- 外部类型：`Id`
- 外部类型：`If`
- 外部类型：`Index`
- 外部类型：`Integrity`
- 外部类型：`Iterator`
- 外部类型：`Load`
- 外部类型：`Meta`
- 外部类型：`Read`
- 外部类型：`Real`
- 外部类型：`Return`
- 外部类型：`Set`
- 外部类型：`Size`
- 外部类型：`So`
- 外部类型：`This`
- 外部类型：`True`
- 外部类型：`Use`
- 外部类型：`Used`
- 外部类型：`Vec1`
- 外部类型：`Vec1Buffer`
- 外部类型：`Vec1Csr`
- 外部类型：`Vec1Filter`
- 外部类型：`Vec1ListBuilder`
- 外部类型：`Vec1MetaBuilder`
- 外部类型：`Vec1MetaValue`
- 外部类型：`Vec1Model`
- 外部类型：`Vec1Tab`
- 外部类型：`Vec1Writer`
- 外部类型：`Writer`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-f134f5b6f8` → 本草稿移入 `cases/defect/auto-sqlite-f134f5b6f8/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
