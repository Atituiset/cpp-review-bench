# auto-sqlite-03af5ac3fa

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#1a16a44e9598c8757c05f125b4ae0061a61810bf](https://github.com/sqlite/sqlite/commit/1a16a44e9598c8757c05f125b4ae0061a61810bf) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-09 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 325 |
| 编译错误数（gcc syntax-only） | 54（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1a16a44e9598c8757c05f125b4ae0061a61810bf (https://github.com/sqlite/sqlite/commit/1a16a44e9598c8757c05f125b4ae0061a61810bf)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 10（原始 PR diff 行 10530；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 1a16a44e9598c8757c05f125b4ae0061a61810bf Latest upstream preprocessor to fix missing file/line error info in some contexts. :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -193,6 +193,9 @@ typedef uint32_t cmpp_flag32_t;
    it is invoked once for each entry and passed 3 arguments: the enum
    entry's full name (CMPP_RC_...), its integer value, and a terse
    informational string.
+
+   The values, aside from CMPP_RC_OK==0, are not fixed and may change
+   in any given build of this library.
 */
 #define cmpp_rc_e_map(E) \
   E(CMPP_RC_OK, 0,                                                      \
@@ -256,10 +259,10 @@ typedef uint32_t cmpp_flag32_t;
    for CMPP_RC_OK, which is guaranteed to always be 0 (and the API
    guarantees that no other code shall have a value of zero).
 
-   The only reasons numbers are hard-coded to the values is to
-   simplify debugging during development. Clients may use
-   cmpp_rc_cstr() to get some human-readable (or programmer-readable)
-   form for any given value in this enum.
+   The only reason numbers are hard-coded to the values is to simplify
+   debugging during development. Clients may use cmpp_rc_cstr() to get
+   some human-readable (or programmer-readable) form for any given
+   value in this enum.
 */
 enum cmpp_rc_e {
 #define E(N,V,H) N = V,
@@ -273,7 +276,7 @@ typedef enum cmpp_rc_e cmpp_rc_e;
    it's not a member of that enum.
 */
 char const * cmpp_rc_cstr(int rc);
-#define cmpp_rc_ucstr(RC) (unsigned char const*)cmpp_rc_cstr(RC)
+#define cmpp_rc_cstru(RC) (unsigned char const*)cmpp_rc_cstr(RC)
 
 /**
    CMPP_BITNESS specifies whether the library should use 32- or 64-bit
@@ -299,7 +302,8 @@ typedef uint32_t cmpp_size_t;
 
   Maintenance reminder: this type must remain 32-bits or risks
   breaking countless printf-style formatting uses, where it frequently
-  ends up being used with "%.*s".
+  ends up being used with "%.*s" (after normalization to a positive
+  value).
 */
 typedef int32_t cmpp_strlen_t;
 
@@ -662,6 +666,40 @@ CMPP_EXPORT int cmpp_undef_legacy(cmpp *pp, const char * zKey,
 CMPP_EXPORT int cmpp_undefine(cmpp *pp, const char * zKey,
                               cmpp_strlen_t n);
 
+/**
+   Searches for a defined value with the key [zName,zName+nName) in
+   pp, using strlen(zName) if nName is negative.
+
+   If pp has error state when this is called, this function is a
+   no-op, returning that code and setting *got to false.
+
+   If found, it is appended to b (but it may have a length of 0, so b
+   might not actually be modified), *got is set to true, and 0 is
+   returned.
+
+   If it is not found, b is not modified and *got is set to false.
+
+   Whether it returns non-0 (an error) if the define is not found
+   depends on (A) enforceUndefPolicy and (B) cmpp_unpol_get():
+
+   - If enforceUndefPolicy policy is false then 0 is returned if no
+   entry is found.
+
+   - If cmpp_unpol_get() indicates that undefined values are not an
+   error, then 0 is returned.
+
+   - If cmpp_unpol_get() indicates that undefined values are an error
+   then a CMPP_RC_NOT_DEFINED error is set in pp and that code is
+   returned.
+
+   Results are undefined if any input pointer is NULL or otherwise
+   invalid.
+*/
+int cmpp_define_get(cmpp *pp, unsigned const char * zName,
+                    cmpp_strlen_t nName, cmpp_b * b,
+                    bool * got,
+                    bool enforceUndefPolicy);
+
 /**
    This works similarly to cmpp_define_v2() except that the new define
    "shadows", rather than overwrites, an existing define with the same
@@ -1801,8 +1839,8 @@ enum cmpp_tt_mask_e {
 };
 
 /**
-   cmpp token types, currently distinct from the public API's cmpp_tt_map
-   as we consolidate the two.
+   An X-macro specifying cmpp token type IDs. Invokes E(X,Y,Z) one
+   time for each entry.
 
    The first argument to E is the tail of the cmpp_tt_ ## TAIL part
    of the enum entry's name.  The second is the cmpp_tt_mask_id part
@@ -1938,12 +1976,12 @@ typedef struct cmpp_tizer cmpp_tizer;
 char const * cmpp_token_cstr3(cmpp_token const * t, cmpp_size_t * n, bool inner);
 
 int cmpp_token_strcmp(cmpp_token const * t, char c
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`_reserve`
- 外部函数：`assert`
- 外部函数：`cases`
- 外部函数：`ceil`
- 外部函数：`cmpp__dx_pi`
- 外部函数：`cmpp__epol`
- 外部函数：`cmpp__err`
- 外部函数：`cmpp__pi`
- 外部函数：`cmpp__policy`
- 外部函数：`cmpp__sqlite3_str_new`
- 外部函数：`cmpp__strlen`
- 外部函数：`cmpp_arg_cstr`
- 外部函数：`cmpp_arg_cstr2`
- 外部函数：`cmpp_arg_ucstr`
- 外部函数：`cmpp_arg_ucstr2`
- 外部函数：`cmpp_atdelim_push`
- 外部函数：`cmpp_b_append`
- 外部函数：`cmpp_b_append4`
- 外部函数：`cmpp_b_append4_str`
- 外部函数：`cmpp_b_append_i64`
- 外部函数：`cmpp_b_append_json_value`
- 外部函数：`cmpp_check_oom`
- 外部函数：`cmpp_compare`
- 外部函数：`cmpp_count_nl`
- 外部函数：`cmpp_db_rc`
- 外部函数：`cmpp_db_rc_v2`
- 外部函数：`cmpp_define_shadow`
- 外部函数：`cmpp_define_unshadow`
- 外部函数：`cmpp_define_v2`
- 外部函数：`cmpp_delimiter_push`
- 外部函数：`cmpp_dx_src_pos_info`
- 外部函数：`cmpp_err`
- 外部函数：`cmpp_err_c`
- 外部函数：`cmpp_err_has`
- 外部函数：`cmpp_errinfo_reuse`
- 外部函数：`cmpp_errinfo_setv`
- 外部函数：`cmpp_errno_rc`
- 外部函数：`cmpp_file_exists`
- 外部函数：`cmpp_is_int64`
- 外部函数：`cmpp_json_is_object`
- 外部函数：`cmpp_json_valid`
- 外部函数：`cmpp_mfree`
- 外部函数：`cmpp_mrealloc`
- 外部函数：`cmpp_rc_cstr`
- 外部函数：`cmpp_rc_e_map`
- 外部函数：`cmpp_rc_ucstr`
- 外部函数：`cmpp_sqlite3_str_finish`
- 外部函数：`cmpp_stmt_bind_int`
- 外部函数：`cmpp_stmt_bind_null`
- 外部函数：`cmpp_stmt_bind_textn`
- 外部函数：`cmpp_stmt_bind_textx`
- 外部函数：`cmpp_stmt_reset`
- 外部函数：`cmpp_stmt_step`
- 外部函数：`cmpp_token_cstr3`
- 外部函数：`cmpp_token_strcmp`
- 外部函数：`cmpp_token_ucstr3`
- 外部函数：`cmpp_token_ustrcmp`
- 外部函数：`cmpp_truthy`
- 外部函数：`cmpp_tt_map`
- 外部函数：`cmpp_undefine`
- 外部函数：`coalesce`
- 外部函数：`def`
- 外部函数：`exit`
- 外部函数：`fails`
- 外部函数：`feof`
- 外部函数：`fflush`
- 外部函数：`floor`
- 外部函数：`fprintf`
- 外部函数：`fputc`
- 外部函数：`fread`
- 外部函数：`g_debug`
- 外部函数：`g_warn`
- 外部函数：`generate_series`
- 外部函数：`incl`
- 外部函数：`inclpath`
- 外部函数：`json_valid`
- 外部函数：`library`
- 外部函数：`main`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`modpath`
- 外部函数：`name`
- 外部函数：`out`
- 外部函数：`predef`
- 外部函数：`readable`
- 外部函数：`replace`
- 外部函数：`s`
- 外部函数：`sdef`
- 外部函数：`sqlite3_busy_timeout`
- 外部函数：`sqlite3_close`
- 外部函数：`sqlite3_column_bytes`
- 外部函数：`sqlite3_column_text`
- 外部函数：`sqlite3_create_function`
- 外部函数：`sqlite3_create_module`
- 外部函数：`sqlite3_db_config`
- 外部函数：`sqlite3_declare_vtab`
- 外部函数：`sqlite3_errcode`
- 外部函数：`sqlite3_exec`
- 外部函数：`sqlite3_expanded_sql`
- 外部函数：`sqlite3_finalize`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_libversion_number`
- 外部函数：`sqlite3_malloc`
- 外部函数：`sqlite3_mprintf`
- 外部函数：`sqlite3_open_v2`
- 外部函数：`sqlite3_prepare_v2`
- 外部函数：`sqlite3_result_int`
- 外部函数：`sqlite3_result_int64`
- 外部函数：`sqlite3_str_append`
- 外部函数：`sqlite3_str_appendall`
- 外部函数：`sqlite3_str_appendchar`
- 外部函数：`sqlite3_str_appendf`
- 外部函数：`sqlite3_str_errcode`
- 外部函数：`sqlite3_str_finish`
- 外部函数：`sqlite3_str_length`
- 外部函数：`sqlite3_str_new`
- 外部函数：`sqlite3_str_vappendf`
- 外部函数：`sqlite3_trace_v2`
- 外部函数：`sqlite3_value_bytes`
- 外部函数：`sqlite3_value_double`
- 外部函数：`sqlite3_value_int`
- 外部函数：`sqlite3_value_int64`
- 外部函数：`sqlite3_value_numeric_type`
- 外部函数：`sqlite3_value_text`
- 外部函数：`sqlite3_value_type`
- 外部函数：`sqlite3_vtab_config`
- 外部函数：`stat`
- 外部函数：`strcmp`
- 外部函数：`strlen`
- 外部函数：`strncmp`
- 外部函数：`to`
- 外部函数：`ttype`
- 外部函数：`udf_compare`
- 外部函数：`use`
- 外部函数：`ustr_c`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 外部函数：`vdef`
- 外部函数：`vfprintf`
- 外部函数：`void`
- 外部函数：`way`
- 外部函数：`write`
- 外部函数：`x`
- 大写宏：`ALL`
- 大写宏：`AND`
- 大写宏：`API`
- 大写宏：`ARG`
- 大写宏：`ATTACH`
- 大写宏：`AUTOINCREMENT`
- 大写宏：`BEGIN`
- 大写宏：`CAST`
- 大写宏：`CMPP_BITNESS`
- 大写宏：`CMPP_CTOR_INSTANCE_INIT`
- 大写宏：`CMPP_RC_`
- 大写宏：`CMPP_RC_ACCESS`
- 大写宏：`CMPP_RC_CORRUPT`
- 大写宏：`CMPP_RC_DB`
- 大写宏：`CMPP_RC_IO`
- 大写宏：`CMPP_RC_MISUSE`
- 大写宏：`CMPP_RC_NOT_DEFINED`
- 大写宏：`CMPP_RC_NOT_FOUND`
- 大写宏：`CMPP_RC_OK`
- 大写宏：`CMPP_RC_OOM`
- 大写宏：`CMPP_RC_RANGE`
- 大写宏：`CMPP_RC_UNSUPPORTED`
- 大写宏：`CMPP__EXPORT`
- 大写宏：`COMMIT`
- 大写宏：`CONFLICT`
- 大写宏：`CREATE`
- 大写宏：`DEFAULT`
- 大写宏：`DELETE`
- 大写宏：`DESC`
- 大写宏：`DETACH`
- 大写宏：`DROP`
- 大写宏：`EXCLUDED`
- 大写宏：`EXCLUSIVE`
- 大写宏：`EXISTS`
- 大写宏：`FAIL`
- 大写宏：`FROM`
- 大写宏：`GLOB`
- 大写宏：`IGNORE`
- 大写宏：`INSERT`
- 大写宏：`INTEGER`
- 大写宏：`INTO`
- 大写宏：`JSON`
- 大写宏：`JSON5`
- 大写宏：`KEY`
- 大写宏：`LHS`
- 大写宏：`LIMIT`
- 大写宏：`MARKER`
- 大写宏：`MSG`
- 大写宏：`NOT`
- 大写宏：`NOTHING`
- 大写宏：`NUL`
- 大写宏：`NULL`
- 大写宏：`OFFSET`
- 大写宏：`OOM`
- 大写宏：`ORDER`
- 大写宏：`PRIMARY`
- 大写宏：`RELEASE`
- 大写宏：`RETURNING`
- 大写宏：`RHS`
- 大写宏：`ROLLBACK`
- 大写宏：`ROWID`
- 大写宏：`SAVEPOINT`
- 大写宏：`SELECT`
- 大写宏：`SET`
- 大写宏：`SQL`
- 大写宏：`SQLITE_AUTH`
- 大写宏：`SQLITE_BLOB`
- 大写宏：`SQLITE_BUSY`
- 大写宏：`SQLITE_CANTOPEN`
- 大写宏：`SQLITE_CORRUPT`
- 大写宏：`SQLITE_DBCONFIG_MAINDBNAME`
- 大写宏：`SQLITE_DETERMINISTIC`
- 大写宏：`SQLITE_DIRECTONLY`
- 大写宏：`SQLITE_DONE`
- 大写宏：`SQLITE_ERROR`
- 大写宏：`SQLITE_FLOAT`
- 大写宏：`SQLITE_FULL`
- 大写宏：`SQLITE_INTEGER`
- 大写宏：`SQLITE_IOERR`
- 大写宏：`SQLITE_LOCKED`
- 大写宏：`SQLITE_NOLFS`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_NOTFOUND`
- 大写宏：`SQLITE_NULL`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_OMIT_VIRTUALTABLE`
- 大写宏：`SQLITE_OPEN_CREATE`
- 大写宏：`SQLITE_OPEN_READWRITE`
- 大写宏：`SQLITE_PERM`
- 大写宏：`SQLITE_RANGE`
- 大写宏：`SQLITE_READONLY`
- 大写宏：`SQLITE_ROW`
- 大写宏：`SQLITE_STATIC`
- 大写宏：`SQLITE_TEXT`
- 大写宏：`SQLITE_TOOBIG`
- 大写宏：`SQLITE_TRACE_STMT`
- 大写宏：`SQLITE_UTF8`
- 大写宏：`SQLITE_VTAB_INNOCUOUS`
- 大写宏：`STR`
- 大写宏：`S_ISREG`
- 大写宏：`TABLE`
- 大写宏：`TAIL`
- 大写宏：`TEXT`
- 大写宏：`TODO`
- 大写宏：`UDF`
- 大写宏：`UNION`
- 大写宏：`UNIQUE`
- 大写宏：`UPDATE`
- 大写宏：`UTC`
- 大写宏：`VALUES`
- 大写宏：`VIEW`
- 大写宏：`WHERE`
- 大写宏：`WHICH`
- 大写宏：`WITHOUT`
- 大写宏：`ZMSG`
- 外部类型：`APIs`
- 外部类型：`An`
- 外部类型：`Anything`
- 外部类型：`Appends`
- 外部类型：`Backtick`
- 外部类型：`BufSize`
- 外部类型：`Caveat`
- 外部类型：`Clients`
- 外部类型：`CmppDLine`
- 外部类型：`CmppLvlList`
- 外部类型：`CmppStmt`
- 外部类型：`CmppStmt_`
- 外部类型：`CmppStmt_defGet`
- 外部类型：`CmppStmt_defSelAll`
- 外部类型：`CmppStmt_e`
- 外部类型：`CmppStmt_insTtype`
- 外部类型：`CmppStmt_none`
- 外部类型：`CmppStmt_spBegin`
- 外部类型：`CmppStmt_spRelease`
- 外部类型：`CmppStmt_spRollback`
- 外部类型：`Compare`
- 外部类型：`Convenience`
- 外部类型：`Current`
- 外部类型：`Currently`
- 外部类型：`Decimal`
- 外部类型：`Drop`
- 外部类型：`Else`
- 外部类型：`Empty`
- 外部类型：`Error`
- 外部类型：`Expects`
- 外部类型：`For`
- 外部类型：`If`
- 外部类型：`Ignore`
- 外部类型：`Initialization`
- 外部类型：`Input`
- 外部类型：`Insert`
- 外部类型：`Installing`
- 外部类型：`It`
- 外部类型：`Keep`
- 外部类型：`Key`
- 外部类型：`Maintenance`
- 外部类型：`Narrow`
- 外部类型：`No`
- 外部类型：`None`
- 外部类型：`Number`
- 外部类型：`Offending`
- 外部类型：`PRIu32`
- 外部类型：`Returns`
- 外部类型：`SQLite`
- 外部类型：`Schema`
- 外部类型：`See`
- 外部类型：`StringBT`
- 外部类型：`StringDQ`
- 外部类型：`StringSQ`
- 外部类型：`Symbolic`
- 外部类型：`T`
- 外部类型：`The`
- 外部类型：`Then`
- 外部类型：`This`
- 外部类型：`To`
- 外部类型：`Transfer`
- 外部类型：`True`
- 外部类型：`UDFs`
- 外部类型：`Unused`
- 外部类型：`Updated`
- 外部类型：`We`
- 外部类型：`Whole`
- 外部类型：`Wonky`
- 外部类型：`Word`
- 外部类型：`Write`
- 外部类型：`cmpp`
- 外部类型：`cmpp__dx_pimpl`
- 外部类型：`cmpp__pimpl`
- 外部类型：`cmpp_arg`
- 外部类型：`cmpp_b`
- 外部类型：`cmpp_dx`
- 外部类型：`cmpp_dx_pos`
- 外部类型：`cmpp_errinfo`
- 外部类型：`cmpp_outputer`
- 外部类型：`cmpp_token`
- 外部类型：`entry`
- 外部类型：`int32_t`
- 外部类型：`int64_t`
- 外部类型：`series_cursor`
- 外部类型：`ssize_t`
- 外部类型：`stat`
- 外部类型：`uint32_t`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-03af5ac3fa` → 本草稿移入 `cases/defect/auto-sqlite-03af5ac3fa/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
