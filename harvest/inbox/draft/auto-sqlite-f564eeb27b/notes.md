# auto-sqlite-f564eeb27b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#bf181220a009b89d782cf6bf9a933683255f7ead](https://github.com/sqlite/sqlite/commit/bf181220a009b89d782cf6bf9a933683255f7ead) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-12 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 76 |
| 编译错误数（gcc syntax-only） | 138（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #bf181220a009b89d782cf6bf9a933683255f7ead (https://github.com/sqlite/sqlite/commit/bf181220a009b89d782cf6bf9a933683255f7ead)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 571；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT bf181220a009b89d782cf6bf9a933683255f7ead Still more hardening against smuggled shell command attacks. :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -277,6 +277,30 @@ static char **tableColumnList(DState *p, const char *zTab){
   return 0;
 }
 
+/*
+** Return a copy of the text z, obtained from sqlite3_malloc(), that is safe
+** to embed inside of a C-style comment in a script of SQL statements and
+** dot-commands.  Newlines and other control characters are changed to spaces,
+** and any end-of-comment sequence is broken up.  Otherwise, text taken from
+** an untrusted database (for example, a table name in an error message)
+** could terminate the comment and put arbitrary SQL, or a line that begins
+** with "." and so is interpreted as a dot-command, into the output.
+*/
+static char *safeCommentText(const char *z){
+  char *zOut = sqlite3_mprintf("%s", z ? z : "");
+  int i;
+  if( zOut==0 ) return 0;
+  for(i=0; zOut[i]; i++){
+    unsigned char c = (unsigned char)zOut[i];
+    if( c<' ' || c==0x7f ){
+      zOut[i] = ' ';
+    }else if( c=='/' && i>0 && zOut[i-1]=='*' ){
+      zOut[i] = ' ';
+    }
+  }
+  return zOut;
+}
+
 /*
 ** Send mprintf-formatted content to the output callback.
 */
@@ -568,8 +592,9 @@ static void output_sql_from_query(
   rc = sqlite3_prepare_v2(p->db, zSql, -1, &pSelect, 0);
   sqlite3_free(zSql);
   if( rc!=SQLITE_OK || !pSelect ){
-    output_formatted(p, "/**** ERROR: (%d) %s *****/\n", rc,
-                sqlite3_errmsg(p->db));
+    char *zMsg = safeCommentText(sqlite3_errmsg(p->db));
+    output_formatted(p, "/**** ERROR: (%d) %s *****/\n", rc, zMsg);
+    sqlite3_free(zMsg);
     p->nErr++;
     return;
   }
@@ -593,8 +618,9 @@ static void output_sql_from_query(
   }
   rc = sqlite3_finalize(pSelect);
   if( rc!=SQLITE_OK ){
-    output_formatted(p, "/**** ERROR: (%d) %s *****/\n", rc,
-                     sqlite3_errmsg(p->db));
+    char *zMsg = safeCommentText(sqlite3_errmsg(p->db));
+    output_formatted(p, "/**** ERROR: (%d) %s *****/\n", rc, zMsg);
+    sqlite3_free(zMsg);
     if( (rc&0xff)!=SQLITE_CORRUPT ) p->nErr++;
   }
 }
@@ -620,7 +646,9 @@ static void run_schema_dump_query(
   sqlite3_exec(p->db, z, dump_callback, p, &zErr);
   sqlite3_free(z);
   if( zErr ){
-    output_formatted(p, "/****** %s ******/\n", zErr);
+    char *zMsg = safeCommentText(zErr);
+    output_formatted(p, "/****** %s ******/\n", zMsg);
+    sqlite3_free(zMsg);
     sqlite3_free(zErr);
     p->nErr++;
     zErr = 0;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`char`
- 外部函数：`isalnum`
- 外部函数：`isalpha`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`pragma_index_list`
- 外部函数：`replace`
- 外部函数：`sqlite3_column_blob`
- 外部函数：`sqlite3_column_bytes`
- 外部函数：`sqlite3_column_double`
- 外部函数：`sqlite3_column_int`
- 外部函数：`sqlite3_column_int64`
- 外部函数：`sqlite3_column_text`
- 外部函数：`sqlite3_column_type`
- 外部函数：`sqlite3_errmsg`
- 外部函数：`sqlite3_exec`
- 外部函数：`sqlite3_finalize`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_keyword_check`
- 外部函数：`sqlite3_mprintf`
- 外部函数：`sqlite3_prepare_v2`
- 外部函数：`sqlite3_realloc64`
- 外部函数：`sqlite3_snprintf`
- 外部函数：`sqlite3_step`
- 外部函数：`sqlite3_strglob`
- 外部函数：`sqlite3_stricmp`
- 外部函数：`sqlite3_table_column_metadata`
- 外部函数：`sqlite3_vmprintf`
- 外部函数：`sqlite_schema`
- 外部函数：`strcmp`
- 外部函数：`strlen`
- 外部函数：`strncmp`
- 外部函数：`strstr`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 外部函数：`xCallback`
- 大写宏：`ANALYZE`
- 大写宏：`CREATE`
- 大写宏：`DELETE`
- 大写宏：`DESC`
- 大写宏：`ERROR`
- 大写宏：`EXISTS`
- 大写宏：`FROM`
- 大写宏：`INSERT`
- 大写宏：`INTEGER`
- 大写宏：`INTO`
- 大写宏：`KEY`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 大写宏：`PRAGMA`
- 大写宏：`PRIMARY`
- 大写宏：`ROWID`
- 大写宏：`SELECT`
- 大写宏：`SQLITE_BLOB`
- 大写宏：`SQLITE_CORRUPT`
- 大写宏：`SQLITE_FLOAT`
- 大写宏：`SQLITE_INTEGER`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_NULL`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_ROW`
- 大写宏：`SQLITE_TEXT`
- 大写宏：`TABLE`
- 大写宏：`VALUES`
- 大写宏：`VIRTUAL`
- 大写宏：`WHERE`
- 大写宏：`WITHOUT`
- 外部类型：`Always`
- 外部类型：`At`
- 外部类型：`Build`
- 外部类型：`But`
- 外部类型：`DState`
- 外部类型：`DText`
- 外部类型：`Ex`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Number`
- 外部类型：`Only`
- 外部类型：`Result`
- 外部类型：`Send`
- 外部类型：`Space`
- 外部类型：`The`
- 外部类型：`There`
- 外部类型：`To`
- 外部类型：`True`
- 外部类型：`Try`
- 外部类型：`Verify`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-f564eeb27b` → 本草稿移入 `cases/defect/auto-sqlite-f564eeb27b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
