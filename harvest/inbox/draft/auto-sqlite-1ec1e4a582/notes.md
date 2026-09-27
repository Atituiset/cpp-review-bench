# auto-sqlite-1ec1e4a582

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#f2375631927a6893f9fcdb6a7c05bade769f53b8](https://github.com/sqlite/sqlite/commit/f2375631927a6893f9fcdb6a7c05bade769f53b8) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-27 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 16 |
| 编译错误数（gcc syntax-only） | 9（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #f2375631927a6893f9fcdb6a7c05bade769f53b8 (https://github.com/sqlite/sqlite/commit/f2375631927a6893f9fcdb6a7c05bade769f53b8)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 646；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT f2375631927a6893f9fcdb6a7c05bade769f53b8 Never let the SQLITE_FP_PRECISION_LIMIT exceed 100 million. :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -187,9 +187,16 @@ static char *printfTempBuf(sqlite3_str *pAccum, sqlite3_int64 n){
 #define etBUFSIZE SQLITE_PRINT_BUF_SIZE  /* Size of the output buffer */
 
 /*
-** Hard limit on the precision of floating-point conversions.
+** Hard limit on the precision of floating-point conversions to
+** the minimum of SQLITE_PRINTF_PRECISION_LIMIT and 100 million.
 */
-#ifndef SQLITE_PRINTF_PRECISION_LIMIT
+#ifdef SQLITE_PRINTF_PRECISION_LIMIT
+# if SQLITE_PRINTF_PRECISION_LIMIT<100000000
+#   define SQLITE_FP_PRECISION_LIMIT SQLITE_PRINTF_PRECISION_LIMIT
+# else
+#   define SQLITE_FP_PRECISION_LIMIT 100000000
+# endif
+#else
 # define SQLITE_FP_PRECISION_LIMIT 100000000
 #endif
 
@@ -549,11 +556,9 @@ void sqlite3_str_vappendf(
           realvalue = va_arg(ap,double);
         }
         if( precision<0 ) precision = 6;         /* Set default precision */
-#ifdef SQLITE_FP_PRECISION_LIMIT
         if( precision>SQLITE_FP_PRECISION_LIMIT ){
           precision = SQLITE_FP_PRECISION_LIMIT;
         }
-#endif
         if( xtype==etFLOAT ){
           iRound = -precision;
         }else if( xtype==etGENERIC ){
@@ -643,7 +648,7 @@ void sqlite3_str_vappendf(
             /* Unable to allocate space in pAccum, perhaps because it
             ** is coming from sqlite3_snprintf() or similar.  We'll have
             ** to render into temporary space and the memcpy() it over. */
-            bufpt = sqlite3_malloc(szBufNeeded);
+            bufpt = sqlite3_malloc64(szBufNeeded);
             if( bufpt==0 ){
               sqlite3StrAccumSetError(pAccum, SQLITE_NOMEM);
               return;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`isMalloced`
- 外部函数：`memcpy`
- 外部函数：`sqlite3DbFree`
- 外部函数：`sqlite3ErrorToParser`
- 外部函数：`sqlite3_malloc`
- 外部函数：`sqlite3_snprintf`
- 外部函数：`va_arg`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_PRINTF_MALLOCED`
- 大写宏：`SQLITE_PRINTF_PRECISION_LIMIT`
- 大写宏：`SQLITE_TOOBIG`
- 外部类型：`Floating`
- 外部类型：`Hard`
- 外部类型：`Set`
- 外部类型：`Size`
- 外部类型：`StrAccum`
- 外部类型：`Unable`
- 外部类型：`We`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-1ec1e4a582` → 本草稿移入 `cases/defect/auto-sqlite-1ec1e4a582/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
