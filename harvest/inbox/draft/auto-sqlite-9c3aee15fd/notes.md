# auto-sqlite-9c3aee15fd

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#e6082d077e7e96c91fa366219ae07a1c5ab0ce70](https://github.com/sqlite/sqlite/commit/e6082d077e7e96c91fa366219ae07a1c5ab0ce70) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-03 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 45 |
| 编译错误数（gcc syntax-only） | 8（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #e6082d077e7e96c91fa366219ae07a1c5ab0ce70 (https://github.com/sqlite/sqlite/commit/e6082d077e7e96c91fa366219ae07a1c5ab0ce70)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT e6082d077e7e96c91fa366219ae07a1c5ab0ce70 When -DSQLITE_DEBUG or -DSQLITE_ENABLE_WALSTAT is used, provide the :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3202,6 +3202,48 @@ static void filestatFunc(
 }
 #endif /* SQLITE_DEBUG || SQLITE_ENABLE_FILESTAT */
 
+#if defined(SQLITE_DEBUG) || defined(SQLITE_ENABLE_WALSTAT)
+/*
+** Implementation of sqlite_walstat(SCHEMA).
+**
+** Return JSON text that describes the current state of a WAL file.
+** This function is for debugging and analysis only.  It is not part
+** of production builds.  This function is not part of the SQLite API
+** and is likely to change or be removed in future versions of SQLite.
+*/
+static void walstatFunc(
+  sqlite3_context *context,
+  int argc,
+  sqlite3_value **argv
+){
+  sqlite3 *db = sqlite3_context_db_handle(context);
+  const char *zDbName;
+  sqlite3_str *pStr;
+  Btree *pBtree;
+
+  zDbName = (const char*)sqlite3_value_text(argv[0]);
+  pBtree = sqlite3DbNameToBtree(db, zDbName);
+  if( pBtree ){
+    Pager *pPager;
+    sqlite3BtreeEnter(pBtree);
+    pPager = sqlite3BtreePager(pBtree);
+    assert( pPager!=0 );
+    pStr = sqlite3_str_new(db);
+    if( sqlite3_str_errcode(pStr) ){
+      sqlite3_result_error_nomem(context);
+    }else{
+      sqlite3_str_append(pStr, "{", 1);
+      sqlite3PagerWalStat(pPager, pStr);
+      sqlite3_str_append(pStr, "}", 1);
+      sqlite3_result_str(context, pStr, SQLITE_FINISH);
+    }
+    sqlite3BtreeLeave(pBtree);
+  }else{
+    sqlite3_result_text(context, "{}", 2, SQLITE_STATIC);
+  }
+}
+#endif /* SQLITE_DEBUG || SQLITE_ENABLE_WALSTAT */
+
 #ifdef SQLITE_DEBUG
 /*
 ** Implementation of fpdecode(x,y,z) function.
@@ -3362,6 +3404,7 @@ void sqlite3RegisterBuiltinFunctions(void){
 #endif
 #if defined(SQLITE_DEBUG) || defined(SQLITE_ENABLE_FILESTAT)
     FUNCTION(sqlite_filestat,    1, 0, 0, filestatFunc     ),
+    FUNCTION(sqlite_walstat,     1, 0, 0, walstatFunc      ),
 #endif
     FUNCTION(ltrim,              1, 1, 0, trimFunc         ),
     FUNCTION(ltrim,              2, 1, 0, trimFunc         ),
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`defined`
- 外部函数：`fpdecode`
- 外部函数：`memcmp`
- 外部函数：`sqlite3BtreeEnter`
- 外部函数：`sqlite3BtreeLeave`
- 外部函数：`sqlite3BtreePager`
- 外部函数：`sqlite3DbNameToBtree`
- 外部函数：`sqlite3Malloc`
- 外部函数：`sqlite3OsFileControl`
- 外部函数：`sqlite3PagerFile`
- 外部函数：`sqlite3PagerJrnlFile`
- 外部函数：`sqlite3_context_db_handle`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_result_error_nomem`
- 外部函数：`sqlite3_result_error_toobig`
- 外部函数：`sqlite3_result_str`
- 外部函数：`sqlite3_result_text`
- 外部函数：`sqlite3_str_append`
- 外部函数：`sqlite3_str_appendall`
- 外部函数：`sqlite3_str_errcode`
- 外部函数：`sqlite3_str_new`
- 外部函数：`sqlite3_user_data`
- 外部函数：`sqlite3_value_bytes`
- 外部函数：`sqlite3_value_text`
- 外部函数：`sqlite3_value_type`
- 外部函数：`testcase`
- 大写宏：`FUNCTION`
- 大写宏：`SQLITE_DEBUG`
- 大写宏：`SQLITE_ENABLE_FILESTAT`
- 大写宏：`SQLITE_FCNTL_FILESTAT`
- 大写宏：`SQLITE_FINISH`
- 大写宏：`SQLITE_LIMIT_LENGTH`
- 大写宏：`SQLITE_NULL`
- 大写宏：`SQLITE_PTR_TO_INT`
- 大写宏：`SQLITE_SKIP_UTF8`
- 大写宏：`SQLITE_STATIC`
- 大写宏：`SQLITE_TRANSIENT`
- 外部类型：`Btree`
- 外部类型：`Implementation`
- 外部类型：`Individual`
- 外部类型：`Input`
- 外部类型：`Length`
- 外部类型：`Loop`
- 外部类型：`Number`
- 外部类型：`Pager`
- 外部类型：`Set`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-9c3aee15fd` → 本草稿移入 `cases/defect/auto-sqlite-9c3aee15fd/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
