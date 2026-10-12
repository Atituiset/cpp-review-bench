# auto-sqlite-5d68d34480

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#891eb44d4dfe8c1afc304d6bf0c17fe7376fd8c1](https://github.com/sqlite/sqlite/commit/891eb44d4dfe8c1afc304d6bf0c17fe7376fd8c1) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-12 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 27 |
| 编译错误数（gcc syntax-only） | 32（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #891eb44d4dfe8c1afc304d6bf0c17fe7376fd8c1 (https://github.com/sqlite/sqlite/commit/891eb44d4dfe8c1afc304d6bf0c17fe7376fd8c1)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 532；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 891eb44d4dfe8c1afc304d6bf0c17fe7376fd8c1 Minor hardening of the fossil delta extension against malformed deltas. Based on suggestions from Dan Mestas. :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -31,7 +31,7 @@
 ** that contain deltas.
 **
 ** Typical build commands assuming
-** 
+**
 **    DIR=ext/misc
 *     NAME=fossildelta
 **
@@ -522,15 +522,15 @@ static int delta_create(
 
 /*
 ** Return the size (in bytes) of the output from applying
-** a delta.
+** a delta or a negative value on error.
 **
 ** This routine is provided so that an procedure that is able
 ** to call delta_apply() can learn how much space is required
-** for the output and hence allocate nor more space that is really
+** for the output and hence allocate no more space than is really
 ** needed.
 */
-static int delta_output_size(const char *zDelta, int lenDelta){
-  int size;
+static sqlite3_int64 delta_output_size(const char *zDelta, int lenDelta){
+  sqlite3_int64 size;
   size = deltaGetInt(&zDelta, &lenDelta);
   if( lenDelta<=0 || *zDelta!='\n' ){
     /* ERROR: size integer not terminated by "\n" */
@@ -692,9 +692,9 @@ static void deltaApplyFunc(
   int argc,
   sqlite3_value **argv
 ){
-  const char *aOrig;   int nOrig;        /* The X input */
-  const char *aDelta;  int nDelta;       /* The input delta (D) */
-  char *aOut;          int nOut, nOut2;  /* The output */
+  const char *aOrig;   int nOrig;          /* The X input */
+  const char *aDelta;  int nDelta;         /* The input delta (D) */
+  char *aOut;          sqlite3_int64 nOut; /* The output */
 
   assert( argc==2 );
   if( sqlite3_value_type(argv[0])==SQLITE_NULL ) return;
@@ -710,11 +710,15 @@ static void deltaApplyFunc(
     sqlite3_result_error(context, "corrupt fossil delta", -1);
     return;
   }
-  aOut = sqlite3_malloc64((sqlite3_int64)nOut+1);
+  if( nOut > 0x7fffffff ){
+    sqlite3_result_error_toobig(context);
+    return;
+  }
+  aOut = sqlite3_malloc64(nOut+1);
   if( aOut==0 ){
     sqlite3_result_error_nomem(context);
   }else{
-    nOut2 = delta_apply(aOrig, nOrig, aDelta, nDelta, aOut);
+    const int nOut2 = delta_apply(aOrig, nOrig, aDelta, nDelta, aOut);
     if( nOut2!=nOut ){
       sqlite3_free(aOut);
       sqlite3_result_error(context, "corrupt fossil delta", -1);
@@ -736,7 +740,7 @@ static void deltaOutputSizeFunc(
   sqlite3_value **argv
 ){
   const char *aDelta;  int nDelta;       /* The input delta (D) */
-  int nOut;                              /* Size of output */
+  sqlite3_int64 nOut;                    /* Size of output */
   assert( argc==1 );
   if( sqlite3_value_type(argv[0])==SQLITE_NULL ) return;
   nDelta = sqlite3_value_bytes(argv[0]);
@@ -748,7 +752,7 @@ static void deltaOutputSizeFunc(
     sqlite3_result_error(context, "corrupt fossil delta", -1);
     return;
   }else{
-    sqlite3_result_int(context, nOut);
+    sqlite3_result_int64(context, nOut);
   }
 }
 
@@ -958,12 +962,12 @@ static int deltaparsevtabColumn(
       break;
     }
     case DELTAPARSEVTAB_A1: {
-      sqlite3_result_int(ctx, pCur->a1);
+      sqlite3_result_int64(ctx, pCur->a1);
       break;
     }
     case DELTAPARSEVTAB_A2: {
       if( pCur->eOp==DELTAPARSE_OP_COPY ){
-        sqlite3_result_int(ctx, pCur->a2);
+        sqlite3_result_int64(ctx, pCur->a2);
       }else if( pCur->eOp==DELTAPARSE_OP_INSERT ){
         if( (i64)pCur->a2 + (i64)pCur->a1 > pCur->nDelta ){
           sqlite3_result_zeroblob(ctx, pCur->a1);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__builtin_bswap32`
- 外部函数：`_byteswap_ulong`
- 外部函数：`assert`
- 外部函数：`delta_output_size`
- 外部函数：`memcpy`
- 大写宏：`DIR`
- 大写宏：`ERROR`
- 大写宏：`FOSSIL_ENABLE_DELTA_CKSUM_TEST`
- 大写宏：`NAME`
- 外部类型：`Delta`
- 外部类型：`Four`
- 外部类型：`Length`
- 外部类型：`Return`
- 外部类型：`Skip`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Typical`
- 外部类型：`Write`
- 外部类型：`size_t`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-5d68d34480` → 本草稿移入 `cases/defect/auto-sqlite-5d68d34480/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
