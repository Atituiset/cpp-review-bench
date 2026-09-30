# auto-sqlite-2625d6801e

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#226a2c0d6ad3f7fb5a291b98c7093b0ce5aba163](https://github.com/sqlite/sqlite/commit/226a2c0d6ad3f7fb5a291b98c7093b0ce5aba163) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-30 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 573 |
| 编译错误数（gcc syntax-only） | 60（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #226a2c0d6ad3f7fb5a291b98c7093b0ce5aba163 (https://github.com/sqlite/sqlite/commit/226a2c0d6ad3f7fb5a291b98c7093b0ce5aba163)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 10（原始 PR diff 行 8823；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 226a2c0d6ad3f7fb5a291b98c7093b0ce5aba163 Generic internal JS cleanups. No functional changes. Update the preprocessor for the ability to use C-style comments in  :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -5,7 +5,7 @@
 
   ./c-pp -I. -I./src -Dsrcdir=./src -o libcmpp.c ./tool/libcmpp.c-pp.c
 
-  with libcmpp 2.0.x e668c06b80424adfc4e2a19a30fa36d1c91071a4a573234336dfab08d29f6add @ 2026-09-15 07:14:55.343 UTC
+  with libcmpp 2.0.x 4539e17f451054d2aeb6a5ffe3891af087fd8fa6cfae4e31d02e1fddfec69d46 @ 2026-09-30 09:12:44.230 UTC
 */
 #if !defined(NET_WANDERINGHORSE_LIBCMPP_C_INCLUDED)
 #define NET_WANDERINGHORSE_LIBCMPP_C_INCLUDED
@@ -20,13 +20,13 @@
 
   ./c-pp -I. -I./src -Dsrcdir=./src -o libcmpp.h ./tool/libcmpp.c-pp.h
 
-  with libcmpp 2.0.x e668c06b80424adfc4e2a19a30fa36d1c91071a4a573234336dfab08d29f6add @ 2026-09-15 07:14:55.343 UTC
+  with libcmpp 2.0.x 4539e17f451054d2aeb6a5ffe3891af087fd8fa6cfae4e31d02e1fddfec69d46 @ 2026-09-30 09:12:44.230 UTC
 */
 #define CMPP_PACKAGE_NAME "libcmpp"
 #define CMPP_LIB_VERSION "2.0.x"
-#define CMPP_LIB_VERSION_HASH "e668c06b80424adfc4e2a19a30fa36d1c91071a4a573234336dfab08d29f6add"
-#define CMPP_LIB_VERSION_TIMESTAMP "2026-09-15 07:14:55.343 UTC"
-#define CMPP_LIB_CONFIG_TIMESTAMP "2026-09-15 13:06 GMT"
+#define CMPP_LIB_VERSION_HASH "4539e17f451054d2aeb6a5ffe3891af087fd8fa6cfae4e31d02e1fddfec69d46"
+#define CMPP_LIB_VERSION_TIMESTAMP "2026-09-30 09:12:44.230 UTC"
+#define CMPP_LIB_CONFIG_TIMESTAMP "2026-09-30 09:14 GMT"
 #define CMPP_VERSION CMPP_LIB_VERSION " " CMPP_LIB_VERSION_HASH " @ " CMPP_LIB_VERSION_TIMESTAMP
 #define CMPP_PLATFORM_EXT_DLL ".so"
 #define CMPP_MODULE_PATH ".:/usr/local/lib/cmpp"
@@ -138,8 +138,6 @@
 #include <stdbool.h>
 #include "sqlite3.h" /* sqlite3_str */
 
-typedef struct cmpp_arg cmpp_arg;
-
 /**
    For loadable modules to be able to portably access the cmpp API,
    without requiring that their loading binary be linked with
@@ -178,6 +176,8 @@ extern "C" {
 typedef struct cmpp__pimpl cmpp__pimpl;
 typedef struct cmpp_api_thunk cmpp_api_thunk;
 typedef struct cmpp_outputer cmpp_outputer;
+typedef struct cmpp_b cmpp_b;
+typedef struct cmpp_arg cmpp_arg;
 typedef void cmpp_itch;
 
 /**
@@ -273,6 +273,7 @@ typedef enum cmpp_rc_e cmpp_rc_e;
    it's not a member of that enum.
 */
 char const * cmpp_rc_cstr(int rc);
+#define cmpp_rc_ucstr(RC) (unsigned char const*)cmpp_rc_cstr(RC)
 
 /**
    CMPP_BITNESS specifies whether the library should use 32- or 64-bit
@@ -295,6 +296,10 @@ typedef uint32_t cmpp_size_t;
   byte ranges in a stream. It is most frequently used in API
   signatures where "if this value is negative then use
   strlen(someOtherArg) to count it".
+
+  Maintenance reminder: this type must remain 32-bits or risks
+  breaking countless printf-style formatting uses, where it frequently
+  ends up being used with "%.*s".
 */
 typedef int32_t cmpp_strlen_t;
 
@@ -618,7 +623,8 @@ CMPP_EXPORT bool cmpp_is_legal_key(unsigned char const *zName,
    Returns 0 on success and updates pp's error state on error.
 
    See: cmpp_define_v2()
-   See: cmpp_undef()
+   See: cmpp_undefine()
+   See: cmpp_undef_legacy()
 */
 CMPP_EXPORT int cmpp_define_legacy(cmpp *pp, const char * zKey,
                                    char const *zVal);
@@ -643,8 +649,18 @@ CMPP_EXPORT int cmpp_define_v2(cmpp *pp, const char * zKey, char const *zVal);
 
    This does _not_ affect defines made using cmpp_define_shadow().
 */
-CMPP_EXPORT int cmpp_undef(cmpp *pp, const char * zKey,
-                           unsigned int *nRemoved);
+CMPP_EXPORT int cmpp_undef_legacy(cmpp *pp, const char * zKey,
+                                  unsigned int *nRemoved);
+/**
+   Undefines the define with the given name (an exact match).  Returns
+   0 on success. If pp
+   has error state, this is a no-op returning the current error code,
+   otherwise it returns non-0 only on allocation error or db-related
+   errors (which, in practice, do not happen for in-memory databases
+   unless they've run out of memory).
+ */
+CMPP_EXPORT int cmpp_undefine(cmpp *pp, const char * zKey,
+                              cmpp_strlen_t n);
 
 /**
    This works similarly to cmpp_define_v2() except that the new d
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__attribute__`
- 外部函数：`access`
- 外部函数：`and`
- 外部函数：`assert`
- 外部函数：`because`
- 外部函数：`bsearch`
- 外部函数：`cases`
- 外部函数：`checkIsDefined`
- 外部函数：`cleanup`
- 外部函数：`cmpp__FileWrapper_open`
- 外部函数：`cmpp__define_impl`
- 外部函数：`cmpp__dx_pi`
- 外部函数：`cmpp__dx_zdelim`
- 外部函数：`cmpp__epol`
- 外部函数：`cmpp__err`
- 外部函数：`cmpp__err_reuse`
- 外部函数：`cmpp__has`
- 外部函数：`cmpp__is_int`
- 外部函数：`cmpp__pi`
- 外部函数：`cmpp__policy`
- 外部函数：`cmpp__pp_zdelim`
- 外部函数：`cmpp__prepare`
- 外部函数：`cmpp__sqlite3_str_new`
- 外部函数：`cmpp__tt_for_sqlite`
- 外部函数：`cmpp_argOp__cmp_bind`
- 外部函数：`cmpp_argOp_for_tt`
- 外部函数：`cmpp_arg_cstr`
- 外部函数：`cmpp_arg_equals`
- 外部函数：`cmpp_arg_equals_c`
- 外部函数：`cmpp_arg_interpolate`
- 外部函数：`cmpp_arg_isflag_c`
- 外部函数：`cmpp_args__not_simplify`
- 外部函数：`cmpp_args_cleanup`
- 外部函数：`cmpp_args_clone`
- 外部函数：`cmpp_args_parse`
- 外部函数：`cmpp_atdelim_get`
- 外部函数：`cmpp_atdelim_pop`
- 外部函数：`cmpp_atdelim_push`
- 外部函数：`cmpp_atdelim_set`
- 外部函数：`cmpp_atpol_get`
- 外部函数：`cmpp_atpol_set`
- 外部函数：`cmpp_b_append`
- 外部函数：`cmpp_b_append4`
- 外部函数：`cmpp_b_append4_str`
- 外部函数：`cmpp_b_append_ch`
- 外部函数：`cmpp_b_append_i32`
- 外部函数：`cmpp_b_append_i64`
- 外部函数：`cmpp_b_append_str`
- 外部函数：`cmpp_b_borrow`
- 外部函数：`cmpp_b_chomp`
- 外部函数：`cmpp_b_clear`
- 外部函数：`cmpp_b_reserve3`
- 外部函数：`cmpp_b_return`
- 外部函数：`cmpp_b_reuse`
- 外部函数：`cmpp_b_swap`
- 外部函数：`cmpp_call_str`
- 外部函数：`cmpp_check_oom`
- 外部函数：`cmpp_chomp`
- 外部函数：`cmpp_compare`
- 外部函数：`cmpp_count_nl`
- 外部函数：`cmpp_d_autoloader_set`
- 外部函数：`cmpp_d_register`
- 外部函数：`cmpp_define_legacy`
- 外部函数：`cmpp_define_shadow`
- 外部函数：`cmpp_define_unshadow`
- 外部函数：`cmpp_define_v2`
- 外部函数：`cmpp_delimiter_pop`
- 外部函数：`cmpp_delimiter_push`
- 外部函数：`cmpp_delimiter_set`
- 外部函数：`cmpp_dx_args_parse`
- 外部函数：`cmpp_dx_consume`
- 外部函数：`cmpp_dx_consume_b`
- 外部函数：`cmpp_dx_delim`
- 外部函数：`cmpp_dx_err`
- 外部函数：`cmpp_dx_pos_restore`
- 外部函数：`cmpp_dx_pos_save`
- 外部函数：`cmpp_dx_src_pos_info`
- 外部函数：`cmpp_err`
- 外部函数：`cmpp_err_get`
- 外部函数：`cmpp_err_has`
- 外部函数：`cmpp_errinfo_reuse`
- 外部函数：`cmpp_errinfo_set`
- 外部函数：`cmpp_errno_rc`
- 外部函数：`cmpp_f`
- 外部函数：`cmpp_f_v4_err_usage`
- 外部函数：`cmpp_file_exists`
- 外部函数：`cmpp_is_int`
- 外部函数：`cmpp_is_int64`
- 外部函数：`cmpp_itch_enum_search`
- 外部函数：`cmpp_kav_each`
- 外部函数：`cmpp_malloc2`
- 外部函数：`cmpp_mfree`
- 外部函数：`cmpp_out_expand`
- 外部函数：`cmpp_out_raw`
- 外部函数：`cmpp_output_f_b`
- 外部函数：`cmpp_process_stream`
- 外部函数：`cmpp_process_string`
- 外部函数：`cmpp_rc_cstr`
- 外部函数：`cmpp_skip_snl`
- 外部函数：`cmpp_skip_snl_trailing`
- 外部函数：`cmpp_skip_space_trailing`
- 外部函数：`cmpp_slurp`
- 外部函数：`cmpp_sp_begin`
- 外部函数：`cmpp_sp_commit`
- 外部函数：`cmpp_sp_rollback`
- 外部函数：`cmpp_stream`
- 外部函数：`cmpp_tizer_dtor`
- 外部函数：`cmpp_tizer_errpos`
- 外部函数：`cmpp_tizer_full_range`
- 外部函数：`cmpp_tizer_init`
- 外部函数：`cmpp_tizer_name`
- 外部函数：`cmpp_tizer_next`
- 外部函数：`cmpp_token_content`
- 外部函数：`cmpp_truthy`
- 外部函数：`cmpp_tt_cstr`
- 外部函数：`cmpp_tt_map`
- 外部函数：`cmpp_undef`
- 外部函数：`cmpp_unpol_set`
- 外部函数：`coalesce`
- 外部函数：`consume`
- 外部函数：`def`
- 外部函数：`dest`
- 外部函数：`dlopen`
- 外部函数：`dtor`
- 外部函数：`dxserr`
- 外部函数：`exit`
- 外部函数：`export_name`
- 外部函数：`f`
- 外部函数：`fails`
- 外部函数：`fclose`
- 外部函数：`feof`
- 外部函数：`fflush`
- 外部函数：`fixme`
- 外部函数：`fopen`
- 外部函数：`fprintf`
- 外部函数：`fputc`
- 外部函数：`fread`
- 外部函数：`g_stderr`
- 外部函数：`g_warn`
- 外部函数：`g_warn0`
- 外部函数：`generate_series`
- 外部函数：`incl`
- 外部函数：`inclpath`
- 外部函数：`it`
- 外部函数：`library`
- 外部函数：`line`
- 外部函数：`lout`
- 外部函数：`main`
- 外部函数：`memcmp`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`modpath`
- 外部函数：`nextArg`
- 外部函数：`object`
- 外部函数：`pOut`
- 外部函数：`popcheck`
- 外部函数：`predef`
- 外部函数：`prefixed`
- 外部函数：`printf`
- 外部函数：`q`
- 外部函数：`recently`
- 外部函数：`replace`
- 外部函数：`returned`
- 外部函数：`s`
- 外部函数：`sdef`
- 外部函数：`serr`
- 外部函数：`should`
- 外部函数：`snprintf`
- 外部函数：`sqlite3_bind_int64`
- 外部函数：`sqlite3_bind_null`
- 外部函数：`sqlite3_bind_parameter_index`
- 外部函数：`sqlite3_bind_text`
- 外部函数：`sqlite3_busy_timeout`
- 外部函数：`sqlite3_changes`
- 外部函数：`sqlite3_clear_bindings`
- 外部函数：`sqlite3_close`
- 外部函数：`sqlite3_column_bytes`
- 外部函数：`sqlite3_column_count`
- 外部函数：`sqlite3_column_int`
- 外部函数：`sqlite3_column_int64`
- 外部函数：`sqlite3_column_name`
- 外部函数：`sqlite3_column_text`
- 外部函数：`sqlite3_column_type`
- 外部函数：`sqlite3_create_function`
- 外部函数：`sqlite3_create_module`
- 外部函数：`sqlite3_data_count`
- 外部函数：`sqlite3_db_config`
- 外部函数：`sqlite3_errcode`
- 外部函数：`sqlite3_errmsg`
- 外部函数：`sqlite3_exec`
- 外部函数：`sqlite3_expanded_sql`
- 外部函数：`sqlite3_finalize`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_libversion_number`
- 外部函数：`sqlite3_mprintf`
- 外部函数：`sqlite3_open_v2`
- 外部函数：`sqlite3_prepare_v2`
- 外部函数：`sqlite3_reset`
- 外部函数：`sqlite3_result_int`
- 外部函数：`sqlite3_result_int64`
- 外部函数：`sqlite3_sql`
- 外部函数：`sqlite3_step`
- 外部函数：`sqlite3_str_append`
- 外部函数：`sqlite3_str_appendall`
- 外部函数：`sqlite3_str_appendchar`
- 外部函数：`sqlite3_str_appendf`
- 外部函数：`sqlite3_str_errcode`
- 外部函数：`sqlite3_str_finish`
- 外部函数：`sqlite3_str_length`
- 外部函数：`sqlite3_str_new`
- 外部函数：`sqlite3_str_vappendf`
- 外部函数：`sqlite3_strglob`
- 外部函数：`sqlite3_trace_v2`
- 外部函数：`sqlite3_value_bytes`
- 外部函数：`sqlite3_value_double`
- 外部函数：`sqlite3_value_int`
- 外部函数：`sqlite3_value_text`
- 外部函数：`sqlite3_value_type`
- 外部函数：`stat`
- 外部函数：`strchr`
- 外部函数：`strlen`
- 外部函数：`strncmp`
- 外部函数：`strtol`
- 外部函数：`strtoll`
- 外部函数：`toBase64`
- 外部函数：`treat`
- 外部函数：`ttype`
- 外部函数：`type`
- 外部函数：`udf_compare`
- 外部函数：`use`
- 外部函数：`used`
- 外部函数：`ustr_c`
- 外部函数：`ustr_nc`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 外部函数：`value`
- 外部函数：`vdef`
- 外部函数：`vfprintf`
- 外部函数：`visibility`
- 外部函数：`void`
- 外部函数：`writing`
- 外部函数：`xCall`
- 外部函数：`z`
- 大写宏：`ALL`
- 大写宏：`AND`
- 大写宏：`API`
- 大写宏：`ARG`
- 大写宏：`ARGS_LIST`
- 大写宏：`ARGS_RAW`
- 大写宏：`ARGS_V4`
- 大写宏：`ATTACH`
- 大写宏：`AUTOINCREMENT`
- 大写宏：`BEGIN`
- 大写宏：`CAST`
- 大写宏：`CFLAGS`
- 大写宏：`CLOSER`
- 大写宏：`CMPP_BITNESS`
- 大写宏：`CMPP_CTOR_INSTANCE_INIT`
- 大写宏：`CMPP_MAIN_AUTOLOADER`
- 大写宏：`CMPP_RC_`
- 大写宏：`CMPP_RC_ACCESS`
- 大写宏：`CMPP_RC_ASSERT`
- 大写宏：`CMPP_RC_CANNOT_HAPPEN`
- 大写宏：`CMPP_RC_CORRUPT`
- 大写宏：`CMPP_RC_DB`
- 大写宏：`CMPP_RC_ERROR`
- 大写宏：`CMPP_RC_IO`
- 大写宏：`CMPP_RC_MISUSE`
- 大写宏：`CMPP_RC_NOT_DEFINED`
- 大写宏：`CMPP_RC_NOT_FOUND`
- 大写宏：`CMPP_RC_NO_DIRECTIVE`
- 大写宏：`CMPP_RC_OOM`
- 大写宏：`CMPP_RC_RANGE`
- 大写宏：`CMPP_RC_SYNTAX`
- 大写宏：`CMPP_RC_TYPE`
- 大写宏：`CMPP_RC_UNSUPPORTED`
- 大写宏：`CMPP_WASM_EXPORT`
- 大写宏：`CMPP__EXPORT`
- 大写宏：`COMMIT`
- 大写宏：`CONFLICT`
- 大写宏：`CREATE`
- 大写宏：`CRNL`
- 大写宏：`DEFAULT`
- 大写宏：`DELETE`
- 大写宏：`DESC`
- 大写宏：`DETACH`
- 大写宏：`DROP`
- 大写宏：`DTRT`
- 大写宏：`END`
- 大写宏：`EOF`
- 大写宏：`EOL`
- 大写宏：`EXCLUDED`
- 大写宏：`EXCLUSIVE`
- 大写宏：`EXISTS`
- 大写宏：`FAIL`
- 大写宏：`FILE`
- 大写宏：`FILENAME`
- 大写宏：`FIXME`
- 大写宏：`FLOW_CONTROL`
- 大写宏：`FROM`
- 大写宏：`GLOB`
- 大写宏：`GMT`
- 大写宏：`HERE`
- 大写宏：`IGNORE`
- 大写宏：`INSERT`
- 大写宏：`INTEGER`
- 大写宏：`INTO`
- 大写宏：`KEY`
- 大写宏：`LHS`
- 大写宏：`LIMIT`
- 大写宏：`LIST`
- 大写宏：`MARKER`
- 大写宏：`MUST`
- 大写宏：`NAME`
- 大写宏：`NDEBUG`
- 大写宏：`NOT`
- 大写宏：`NOTHING`
- 大写宏：`NOT_SIMPLIFY`
- 大写宏：`NUL`
- 大写宏：`NULL`
- 大写宏：`OFLAGS`
- 大写宏：`OOM`
- 大写宏：`OPENER`
- 大写宏：`ORDER`
- 大写宏：`PRIMARY`
- 大写宏：`RAW`
- 大写宏：`README`
- 大写宏：`RELEASE`
- 大写宏：`RETURNING`
- 大写宏：`RHS`
- 大写宏：`ROLLBACK`
- 大写宏：`ROWID`
- 大写宏：`R_OK`
- 大写宏：`SAVEPOINT`
- 大写宏：`SELECT`
- 大写宏：`SEPARATOR`
- 大写宏：`SET`
- 大写宏：`SQL`
- 大写宏：`SQLITE_`
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
- 大写宏：`SQLITE_MAX_LENGTH`
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
- 大写宏：`SQLITE_STEP`
- 大写宏：`SQLITE_TEXT`
- 大写宏：`SQLITE_TOOBIG`
- 大写宏：`SQLITE_TRACE_STMT`
- 大写宏：`SQLITE_TRANSIENT`
- 大写宏：`SQLITE_UTF8`
- 大写宏：`START`
- 大写宏：`STR`
- 大写宏：`STRING`
- 大写宏：`SYMNAME`
- 大写宏：`S_ISREG`
- 大写宏：`TABLE`
- 大写宏：`TEXT`
- 大写宏：`TODO`
- 大写宏：`UDF`
- 大写宏：`UNION`
- 大写宏：`UNIQUE`
- 大写宏：`UPDATE`
- 大写宏：`UTC`
- 大写宏：`VALUES`
- 大写宏：`VIEW`
- 大写宏：`WASM`
- 大写宏：`WHERE`
- 大写宏：`WHICH`
- 大写宏：`WITHOUT`
- 外部类型：`APIs`
- 外部类型：`Add`
- 外部类型：`Adds`
- 外部类型：`An`
- 外部类型：`Anything`
- 外部类型：`Appends`
- 外部类型：`Arguable`
- 外部类型：`As`
- 外部类型：`Assertion`
- 外部类型：`Backtick`
- 外部类型：`Basic`
- 外部类型：`Because`
- 外部类型：`Begin`
- 外部类型：`Bindable`
- 外部类型：`Blob`
- 外部类型：`Cannot`
- 外部类型：`Chained`
- 外部类型：`Clears`
- 外部类型：`CmppArgList`
- 外部类型：`CmppDLine`
- 外部类型：`CmppDList`
- 外部类型：`CmppDList_empty`
- 外部类型：`CmppDList_empty_m`
- 外部类型：`CmppDList_entry`
- 外部类型：`CmppDList_entry_cmp_pp`
- 外部类型：`CmppIfState`
- 外部类型：`CmppLvl`
- 外部类型：`CmppLvl_F_ELIDE`
- 外部类型：`CmppLvl_F_INHERIT_MASK`
- 外部类型：`CmppLvl_empty`
- 外部类型：`CmppStmt_`
- 外部类型：`CmppStmt_cmpVD`
- 外部类型：`CmppStmt_cmpVV`
- 外部类型：`CmppStmt_dbAttach`
- 外部类型：`CmppStmt_dbDetach`
- 外部类型：`CmppStmt_defDel`
- 外部类型：`CmppStmt_defGet`
- 外部类型：`CmppStmt_defGetBool`
- 外部类型：`CmppStmt_defGetInt`
- 外部类型：`CmppStmt_defHas`
- 外部类型：`CmppStmt_defIns`
- 外部类型：`CmppStmt_defSelAll`
- 外部类型：`CmppStmt_e`
- 外部类型：`CmppStmt_inclDel`
- 外部类型：`CmppStmt_inclHas`
- 外部类型：`CmppStmt_inclIns`
- 外部类型：`CmppStmt_inclPathAdd`
- 外部类型：`CmppStmt_inclSearch`
- 外部类型：`CmppStmt_insTtype`
- 外部类型：`CmppStmt_none`
- 外部类型：`CmppStmt_sdefDel`
- 外部类型：`CmppStmt_selPathSearch`
- 外部类型：`CmppStmt_spBegin`
- 外部类型：`CmppStmt_spRelease`
- 外部类型：`CmppStmt_spRollback`
- 外部类型：`Compare`
- 外部类型：`Consume`
- 外部类型：`Create`
- 外部类型：`Creates`
- 外部类型：`Current`
- 外部类型：`Decimal`
- 外部类型：`Directives`
- 外部类型：`Drop`
- 外部类型：`Dsrcdir`
- 外部类型：`Else`
- 外部类型：`Emits`
- 外部类型：`Emitting`
- 外部类型：`Empty`
- 外部类型：`End`
- 外部类型：`Eq`
- 外部类型：`Error`
- 外部类型：`Example`
- 外部类型：`Expecting`
- 外部类型：`Expects`
- 外部类型：`Expressions`
- 外部类型：`Extra`
- 外部类型：`Fall`
- 外部类型：`FileWrapper`
- 外部类型：`FileWrapper_empty`
- 外部类型：`Flush`
- 外部类型：`For`
- 外部类型：`Found`
- 外部类型：`Ge`
- 外部类型：`Gt`
- 外部类型：`Heredoc`
- 外部类型：`Idir`
- 外部类型：`If`
- 外部类型：`Ill`
- 外部类型：`Illegal`
- 外部类型：`Implementations`
- 外部类型：`In`
- 外部类型：`Initialization`
- 外部类型：`Insert`
- 外部类型：`Installing`
- 外部类型：`Invalid`
- 外部类型：`Invalidates`
- 外部类型：`It`
- 外部类型：`Keep`
- 外部类型：`Key`
- 外部类型：`LFs`
- 外部类型：`Le`
- 外部类型：`Line`
- 外部类型：`Look`
- 外部类型：`Lt`
- 外部类型：`Maybe`
- 外部类型：`Mis`
- 外部类型：`Missing`
- 外部类型：`Most`
- 外部类型：`Neq`
- 外部类型：`No`
- 外部类型：`None`
- 外部类型：`Not`
- 外部类型：`Note`
- 外部类型：`Offending`
- 外部类型：`On`
- 外部类型：`Once`
- 外部类型：`Only`
- 外部类型：`Ownership`
- 外部类型：`PRIi64`
- 外部类型：`PRIu32`
- 外部类型：`Pass`
- 外部类型：`Plan`
- 外部类型：`PodList__atpol`
- 外部类型：`Potential`
- 外部类型：`Propagate`
- 外部类型：`Proxy`
- 外部类型：`Push`
- 外部类型：`Re`
- 外部类型：`Reached`
- 外部类型：`Recursive`
- 外部类型：`Remember`
- 外部类型：`Reminder`
- 外部类型：`Resets`
- 外部类型：`Resolving`
- 外部类型：`Return`
- 外部类型：`Returns`
- 外部类型：`Roll`
- 外部类型：`SQLite`
- 外部类型：`Scans`
- 外部类型：`Schema`
- 外部类型：`Script`
- 外部类型：`Searches`
- 外部类型：`See`
- 外部类型：`Set`
- 外部类型：`Similarly`
- 外部类型：`Skip`
- 外部类型：`Start`
- 外部类型：`StringBT`
- 外部类型：`StringDQ`
- 外部类型：`StringSQ`
- 外部类型：`Strip`
- 外部类型：`Symbolic`
- 外部类型：`Synthesize`
- 外部类型：`That`
- 外部类型：`The`
- 外部类型：`Then`
- 外部类型：`There`
- 外部类型：`This`
- 外部类型：`Thus`
- 外部类型：`To`
- 外部类型：`Too`
- 外部类型：`Treat`
- 外部类型：`True`
- 外部类型：`Try`
- 外部类型：`Typically`
- 外部类型：`UDFs`
- 外部类型：`Unexpected`
- 外部类型：`Unhandled`
- 外部类型：`Unknown`
- 外部类型：`Unused`
- 外部类型：`Update`
- 外部类型：`Usage`
- 外部类型：`Very`
- 外部类型：`Wall`
- 外部类型：`We`
- 外部类型：`Werror`
- 外部类型：`Wextra`
- 外部类型：`Whole`
- 外部类型：`Will`
- 外部类型：`Word`
- 外部类型：`Works`
- 外部类型：`Write`
- 外部类型：`cmpp`
- 外部类型：`cmpp__delim`
- 外部类型：`cmpp__dx_pimpl`
- 外部类型：`cmpp__pimpl`
- 外部类型：`cmpp_api_thunk`
- 外部类型：`cmpp_arg`
- 外部类型：`cmpp_argOp`
- 外部类型：`cmpp_args`
- 外部类型：`cmpp_atpol_e`
- 外部类型：`cmpp_b`
- 外部类型：`cmpp_ctor_opt`
- 外部类型：`cmpp_d`
- 外部类型：`cmpp_d_autoloader`
- 外部类型：`cmpp_d_reg`
- 外部类型：`cmpp_dx`
- 外部类型：`cmpp_dx_pos`
- 外部类型：`cmpp_errinfo`
- 外部类型：`cmpp_f_args`
- 外部类型：`cmpp_outputer`
- 外部类型：`cmpp_tizer`
- 外部类型：`cmpp_token`
- 外部类型：`cmpp_tt_e`
- 外部类型：`cmpp_unpol_e`
- 外部类型：`for`
- 外部类型：`int32_t`
- 外部类型：`int64_t`
- 外部类型：`stat`
- 外部类型：`uint32_t`
- 外部类型：`uint8_t`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-2625d6801e` → 本草稿移入 `cases/defect/auto-sqlite-2625d6801e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
