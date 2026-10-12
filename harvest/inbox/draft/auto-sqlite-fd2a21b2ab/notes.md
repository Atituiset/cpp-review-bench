# auto-sqlite-fd2a21b2ab

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#c5115c777078b53e8e6fda4793e6eb50d0f38d15](https://github.com/sqlite/sqlite/commit/c5115c777078b53e8e6fda4793e6eb50d0f38d15) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-12 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 99 |
| 编译错误数（gcc syntax-only） | 70（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #c5115c777078b53e8e6fda4793e6eb50d0f38d15 (https://github.com/sqlite/sqlite/commit/c5115c777078b53e8e6fda4793e6eb50d0f38d15)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 3844；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT c5115c777078b53e8e6fda4793e6eb50d0f38d15 Merge fixes from branch-3.54 :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -398,6 +398,7 @@ struct JsonParse {
 */
 #define JSON_EDITABLE  0x01   /* Generate a writable JsonParse object */
 #define JSON_KEEPERROR 0x02   /* Return non-NULL even if there is an error */
+#define JSON_NOCACHE   0x04   /* Do not use the JSON cache */
 
 /**************************************************************************
 ** Forward references
@@ -3841,7 +3842,9 @@ static JsonParse *jsonParseFuncArg(
   if( eType==SQLITE_NULL ){
     return 0;
   }
-  pFromCache = jsonCacheSearch(ctx, pArg);
+  if( (flgs & JSON_NOCACHE)==0 ){
+    pFromCache = jsonCacheSearch(ctx, pArg);
+  }
   if( pFromCache ){
     pFromCache->nJPRef++;
     if( (flgs & JSON_EDITABLE)==0 ){
@@ -4916,7 +4919,7 @@ static void jsonValidFunc(
       if( (flags & 0x3)==0 ) break;
       memset(&px, 0, sizeof(px));
      
-      p = jsonParseFuncArg(ctx, argv[0], JSON_KEEPERROR);
+      p = jsonParseFuncArg(ctx, argv[0], JSON_KEEPERROR|JSON_NOCACHE);
       if( p ){
         if( p->oom ){
           sqlite3_result_error_nomem(ctx);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`jsonIsspace`
- 外部函数：`memcmp`
- 外部函数：`memcpy`
- 外部函数：`memmove`
- 外部函数：`memset`
- 外部函数：`readfile`
- 外部函数：`sqlite3DbFree`
- 外部函数：`sqlite3DbMallocRaw`
- 外部函数：`sqlite3DbMallocZero`
- 外部函数：`sqlite3DbRealloc`
- 外部函数：`sqlite3Isalnum`
- 外部函数：`sqlite3Isdigit`
- 外部函数：`sqlite3Isxdigit`
- 外部函数：`sqlite3JsonId1`
- 外部函数：`sqlite3JsonId2`
- 外部函数：`sqlite3RCStrNew`
- 外部函数：`sqlite3RCStrRef`
- 外部函数：`sqlite3RCStrUnref`
- 外部函数：`sqlite3StrNICmp`
- 外部函数：`sqlite3Utf8ReadLimited`
- 外部函数：`sqlite3ValueIsOfClass`
- 外部函数：`sqlite3_context_db_handle`
- 外部函数：`sqlite3_get_auxdata`
- 外部函数：`sqlite3_result_error`
- 外部函数：`sqlite3_result_error_nomem`
- 外部函数：`sqlite3_set_auxdata`
- 外部函数：`sqlite3_value_blob`
- 外部函数：`sqlite3_value_bytes`
- 外部函数：`sqlite3_value_text`
- 外部函数：`sqlite3_value_type`
- 外部函数：`strchr`
- 外部函数：`strncmp`
- 外部函数：`strspn`
- 外部函数：`testcase`
- 大写宏：`ALWAYS`
- 大写宏：`CLI`
- 大写宏：`FLOAT`
- 大写宏：`JSON`
- 大写宏：`JSON5`
- 大写宏：`JSONB`
- 大写宏：`JSONB_`
- 大写宏：`NEVER`
- 大写宏：`NO_TEST`
- 大写宏：`NULL`
- 大写宏：`OOM`
- 大写宏：`SQL`
- 大写宏：`SQLITE_ASCII`
- 大写宏：`SQLITE_BLOB`
- 大写宏：`SQLITE_BUG_COMPATIBLE_20250510`
- 大写宏：`SQLITE_DEBUG`
- 大写宏：`SQLITE_EBCDIC`
- 大写宏：`SQLITE_NOINLINE`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_NULL`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_TEXT`
- 外部类型：`An`
- 外部类型：`And`
- 外部类型：`Bit`
- 外部类型：`But`
- 外部类型：`Cache`
- 外部类型：`Checked`
- 外部类型：`Control`
- 外部类型：`Correct`
- 外部类型：`Current`
- 外部类型：`Datatype`
- 外部类型：`End`
- 外部类型：`Error`
- 外部类型：`For`
- 外部类型：`Forward`
- 外部类型：`From`
- 外部类型：`Function`
- 外部类型：`Generate`
- 外部类型：`If`
- 外部类型：`Index`
- 外部类型：`Infinity`
- 外部类型：`Initialize`
- 外部类型：`Input`
- 外部类型：`JsonCache`
- 外部类型：`JsonParse`
- 外部类型：`Legacy`
- 外部类型：`List`
- 外部类型：`Make`
- 外部类型：`Max`
- 外部类型：`Might`
- 外部类型：`NaN`
- 外部类型：`Node`
- 外部类型：`Number`
- 外部类型：`Object`
- 外部类型：`One`
- 外部类型：`Only`
- 外部类型：`Os`
- 外部类型：`Parse`
- 外部类型：`Quirks`
- 外部类型：`Report`
- 外部类型：`Return`
- 外部类型：`SQLite`
- 外部类型：`See`
- 外部类型：`Start`
- 外部类型：`Syntax`
- 外部类型：`Text`
- 外部类型：`That`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Use`
- 外部类型：`Value`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-fd2a21b2ab` → 本草稿移入 `cases/defect/auto-sqlite-fd2a21b2ab/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
