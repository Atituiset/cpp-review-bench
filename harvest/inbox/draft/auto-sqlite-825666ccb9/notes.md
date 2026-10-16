# auto-sqlite-825666ccb9

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#848cad0ec0b9008690af0b3e95b95814875a8c27](https://github.com/sqlite/sqlite/commit/848cad0ec0b9008690af0b3e95b95814875a8c27) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-16 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 40 |
| 编译错误数（gcc syntax-only） | 132（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #848cad0ec0b9008690af0b3e95b95814875a8c27 (https://github.com/sqlite/sqlite/commit/848cad0ec0b9008690af0b3e95b95814875a8c27)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 738；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 848cad0ec0b9008690af0b3e95b95814875a8c27 Deal with harmless 32-bit integer overflow on parameters to some extension :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -713,6 +713,31 @@ static Decimal *decimalFromDouble(double r){
   return pA;
 }
 
+/*
+** Extract a 32-bit signed integer from argument pVal and write the
+** results into *pResult.  Return 0 on success and 1 if the value is
+** too larger to fit in 32-bits.
+*/
+static int decimalSafeInt(
+  sqlite3_context *context,
+  sqlite3_value *pVal,
+  int *pResult
+){
+  sqlite3_int64 i64 = sqlite3_value_int64(pVal);
+  if( i64<-2147483648LL || i64>2147483647LL ){
+    char *zMsg = sqlite3_mprintf(
+      "value %lld is too large: range limit -2147483648..+2147483647",
+      i64);
+    sqlite3_result_error(context, zMsg, -1);
+    sqlite3_free(zMsg);
+    *pResult = 0;
+    return 1;
+  }else{
+    *pResult = (int)i64;
+    return 0;
+  }
+}
+
 /*
 ** SQL Function:   decimal(X)
 ** OR:             decimal_exp(X)
@@ -734,8 +759,12 @@ static void decimalFunc(
   Decimal *p =  decimal_new(context, argv[0], 0);
   int N;
   if( argc==2 ){
-    N = sqlite3_value_int(argv[1]);
-    if( N>0 ) decimal_round(p, N);
+    if( decimalSafeInt(context, argv[1], &N) ){
+      decimal_free(p);
+      p = 0;
+    }else if( N>0 ){
+      decimal_round(p, N);
+    }
   }else{
     N = 0;
   }
@@ -910,9 +939,13 @@ static void decimalPow2Func(
 ){
   UNUSED_PARAMETER(argc);
   if( sqlite3_value_type(argv[0])==SQLITE_INTEGER ){
-    Decimal *pA = decimalPow2(sqlite3_value_int(argv[0]));
-    decimal_result_sci(context, pA, 0);
-    decimal_free(pA);
+    int N;
+    if( decimalSafeInt(context, argv[0], &N) ) return;
+    Decimal *pA = decimalPow2(N);
+    if( pA ){
+      decimal_result_sci(context, pA, 0);
+      decimal_free(pA);
+    }
   }
 }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`decimal`
- 外部函数：`decimal_exp`
- 外部函数：`isspace`
- 外部函数：`memcpy`
- 外部函数：`memmove`
- 外部函数：`memset`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_malloc64`
- 外部函数：`sqlite3_realloc64`
- 外部函数：`sqlite3_result_error_nomem`
- 外部函数：`sqlite3_result_null`
- 外部函数：`sqlite3_result_text`
- 外部函数：`sqlite3_snprintf`
- 外部函数：`sqlite3_value_blob`
- 外部函数：`sqlite3_value_bytes`
- 外部函数：`sqlite3_value_double`
- 外部函数：`sqlite3_value_int`
- 外部函数：`sqlite3_value_text`
- 外部函数：`sqlite3_value_type`
- 外部函数：`strlen`
- 大写宏：`SQL`
- 大写宏：`SQLITE_BLOB`
- 大写宏：`SQLITE_FLOAT`
- 大写宏：`SQLITE_INTEGER`
- 大写宏：`SQLITE_NULL`
- 大写宏：`SQLITE_TEXT`
- 外部类型：`Always`
- 外部类型：`Array`
- 外部类型：`At`
- 外部类型：`Construct`
- 外部类型：`Decimal`
- 外部类型：`Digits`
- 外部类型：`Do`
- 外部类型：`Exit`
- 外部类型：`Exponent`
- 外部类型：`Function`
- 外部类型：`If`
- 外部类型：`Infinity`
- 外部类型：`Loop`
- 外部类型：`Multiplier`
- 外部类型：`NaN`
- 外部类型：`Number`
- 外部类型：`Report`
- 外部类型：`The`
- 外部类型：`Zero`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-825666ccb9` → 本草稿移入 `cases/defect/auto-sqlite-825666ccb9/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
