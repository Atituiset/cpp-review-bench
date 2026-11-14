# auto-sqlite-da29976dc8

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#d3b7c03bdfa9a1bd379f967df73439256b6b68e6](https://github.com/sqlite/sqlite/commit/d3b7c03bdfa9a1bd379f967df73439256b6b68e6) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-14 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 461 |
| 编译错误数（gcc syntax-only） | 344（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #d3b7c03bdfa9a1bd379f967df73439256b6b68e6 (https://github.com/sqlite/sqlite/commit/d3b7c03bdfa9a1bd379f967df73439256b6b68e6)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 10（原始 PR diff 行 14054；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT d3b7c03bdfa9a1bd379f967df73439256b6b68e6 Internal cleanups and bug fixes in the JS preprocessor. :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -5,7 +5,7 @@
 
   ./c-pp -I. -I./src -Dsrcdir=./src -o libcmpp.c ./tool/libcmpp.c-pp.c
 
-  with libcmpp 2.0.x 4539e17f451054d2aeb6a5ffe3891af087fd8fa6cfae4e31d02e1fddfec69d46 @ 2026-09-30 09:12:44.230 UTC
+  with libcmpp 2.0.x e07fd9aca8774e8c6270f31fcfc7fa477c8991d71d4dca34607a48fe4c420463 @ 2026-11-10 22:06:23.225 UTC
 */
 #if !defined(NET_WANDERINGHORSE_LIBCMPP_C_INCLUDED)
 #define NET_WANDERINGHORSE_LIBCMPP_C_INCLUDED
@@ -20,13 +20,13 @@
 
   ./c-pp -I. -I./src -Dsrcdir=./src -o libcmpp.h ./tool/libcmpp.c-pp.h
 
-  with libcmpp 2.0.x 4539e17f451054d2aeb6a5ffe3891af087fd8fa6cfae4e31d02e1fddfec69d46 @ 2026-09-30 09:12:44.230 UTC
+  with libcmpp 2.0.x e07fd9aca8774e8c6270f31fcfc7fa477c8991d71d4dca34607a48fe4c420463 @ 2026-11-10 22:06:23.225 UTC
 */
 #define CMPP_PACKAGE_NAME "libcmpp"
 #define CMPP_LIB_VERSION "2.0.x"
-#define CMPP_LIB_VERSION_HASH "4539e17f451054d2aeb6a5ffe3891af087fd8fa6cfae4e31d02e1fddfec69d46"
-#define CMPP_LIB_VERSION_TIMESTAMP "2026-09-30 09:12:44.230 UTC"
-#define CMPP_LIB_CONFIG_TIMESTAMP "2026-09-30 09:14 GMT"
+#define CMPP_LIB_VERSION_HASH "e07fd9aca8774e8c6270f31fcfc7fa477c8991d71d4dca34607a48fe4c420463"
+#define CMPP_LIB_VERSION_TIMESTAMP "2026-11-10 22:06:23.225 UTC"
+#define CMPP_LIB_CONFIG_TIMESTAMP "2026-11-13 16:57 GMT"
 #define CMPP_VERSION CMPP_LIB_VERSION " " CMPP_LIB_VERSION_HASH " @ " CMPP_LIB_VERSION_TIMESTAMP
 #define CMPP_PLATFORM_EXT_DLL ".so"
 #define CMPP_MODULE_PATH ".:/usr/local/lib/cmpp"
@@ -2659,11 +2659,11 @@ enum cmpp_f_e {
      If cmpp_f_F_ARGS_RAW is used together with this flag then
      it works a bit differently:
 
-     - cmpp_f_args::argc will be set to the number of arguments
-       tokenized by the preprocessor (it has to do so to be able to
-       get to the point of calling the cmpp_f()). Because V4 includes
-       the name of the being-invoked directive or command as argument
-       0, argc will always be at least one.
+     - cmpp_f_args::argc will always be 1, accounting only for the
+       name of the being-invoked directive or command (which V4
+       always includes as argument 0). The preprocessor does not
+       tokenize the remaining arguments in this mode: that is left to
+       the cmpp_f() impl.
 
      - cmpp_f_args::argv will be NULL. Such cmpp_f() impls are
        expected to parse the arguments using (e.g.) cmpp_tizer with
@@ -11948,41 +11948,59 @@ static char * skipNonB64( char *s, int nc ){
   return s;
 }
 
-/* Decode base64 text into a byte buffer. */
+/* Decode base64 text into a byte buffer.
+**
+** Whitespace is ignored wherever it appears, including inside a group
+** of four base64 digits.  A pad character fills one position of its
+** group with zero bits.  Any other non-base64 character inside a group
+** ends the decode.  Each group yields only the whole bytes encoded by
+** the base64 digits actually present in it.
+*/
+/* src: https://sqlite.org/src/info/9de86d3f09325a0ffea */
 static uint8_t* fromBase64( char *pIn, int ncIn, uint8_t *pOut ){
   if( ncIn>0 && pIn[ncIn-1]=='\n' ) --ncIn;
   while( ncIn>0 && *pIn!=PAD_CHAR ){
     static signed char nboi[] = { 0, 0, 1, 2, 3 };
     char *pUse = skipNonB64(pIn, ncIn);
     unsigned long qv = 0L;
-    int nti, nbo, nac;
+    int nbo, nac = 0, ndg = 0;
     ncIn -= (pUse - pIn);
     pIn = pUse;
-    nti = (ncIn>4)? 4 : ncIn;
-    ncIn -= nti;
-    nbo = nboi[nti];
-    if( nbo==0 ) break;
-    for( nac=0; nac<4; ++nac ){
-      char c = (nac<nti)? *pIn++ : b64Numerals[0];
-      uint8_t bdp = BX_DV_PROTO(c);
-      switch( bdp ){
-      case ND:
-        /*  Treat dark non-digits as pad, but they terminate decode too. */
-        ncIn = 0;
-        deliberate_fall_through; /* FALLTHRU */
-      case WS:
-        /* Treat whitespace as pad and terminate this group.*/
-        nti = nac;
-        deliberate_fall_through; /* FALLTHRU */
-      case PC:
-        bdp = 0;
-        --nbo;
-        deliberate_fall_through; /* FALLTHRU */
-      default: /* bdp is the digit value. *
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`_access`
- 外部函数：`access`
- 外部函数：`assert`
- 外部函数：`bsearch`
- 外部函数：`cases`
- 外部函数：`checkIsDefined`
- 外部函数：`cmpp__dx_pi`
- 外部函数：`cmpp__dx_zdelim`
- 外部函数：`cmpp__epol`
- 外部函数：`cmpp__err`
- 外部函数：`cmpp__err_no_dlls`
- 外部函数：`cmpp__err_reuse`
- 外部函数：`cmpp__li_reserve1_size`
- 外部函数：`cmpp__out2`
- 外部函数：`cmpp__pi`
- 外部函数：`cmpp__policy`
- 外部函数：`cmpp__sqlite3_str_new`
- 外部函数：`cmpp_argOp__cmp_bind`
- 外部函数：`cmpp_argOp_for_tt`
- 外部函数：`cmpp_arg_cstr`
- 外部函数：`cmpp_arg_cstr2`
- 外部函数：`cmpp_arg_equals`
- 外部函数：`cmpp_arg_equals_c`
- 外部函数：`cmpp_arg_interpolate`
- 外部函数：`cmpp_arg_isflag_c`
- 外部函数：`cmpp_args__not_simplify`
- 外部函数：`cmpp_args_cleanup`
- 外部函数：`cmpp_args_clone`
- 外部函数：`cmpp_args_parse`
- 外部函数：`cmpp_atdelim_pop`
- 外部函数：`cmpp_atdelim_push`
- 外部函数：`cmpp_atpol_get`
- 外部函数：`cmpp_atpol_push`
- 外部函数：`cmpp_b_append`
- 外部函数：`cmpp_b_append4`
- 外部函数：`cmpp_b_append4_str`
- 外部函数：`cmpp_b_append_ch`
- 外部函数：`cmpp_b_append_i32`
- 外部函数：`cmpp_b_append_i64`
- 外部函数：`cmpp_b_borrow`
- 外部函数：`cmpp_b_chomp`
- 外部函数：`cmpp_b_clear`
- 外部函数：`cmpp_b_reserve3`
- 外部函数：`cmpp_b_return`
- 外部函数：`cmpp_b_reuse`
- 外部函数：`cmpp_b_swap`
- 外部函数：`cmpp_call_str`
- 外部函数：`cmpp_check_oom`
- 外部函数：`cmpp_compare`
- 外部函数：`cmpp_count_nl`
- 外部函数：`cmpp_d_autoloader_set`
- 外部函数：`cmpp_d_register`
- 外部函数：`cmpp_db_rc`
- 外部函数：`cmpp_db_rc_v2`
- 外部函数：`cmpp_define_shadow`
- 外部函数：`cmpp_define_unshadow`
- 外部函数：`cmpp_define_v2`
- 外部函数：`cmpp_delimiter_push`
- 外部函数：`cmpp_delimiter_set`
- 外部函数：`cmpp_dx_consume`
- 外部函数：`cmpp_dx_consume_b`
- 外部函数：`cmpp_dx_delim`
- 外部函数：`cmpp_dx_err`
- 外部函数：`cmpp_dx_is_eliding`
- 外部函数：`cmpp_dx_src_pos_info`
- 外部函数：`cmpp_err`
- 外部函数：`cmpp_err_c`
- 外部函数：`cmpp_err_has`
- 外部函数：`cmpp_errinfo_reuse`
- 外部函数：`cmpp_errinfo_setv`
- 外部函数：`cmpp_errno_rc`
- 外部函数：`cmpp_f_noop`
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
- 外部函数：`cmpp_process_stream`
- 外部函数：`cmpp_process_string`
- 外部函数：`cmpp_rc_cstr`
- 外部函数：`cmpp_skip_snl`
- 外部函数：`cmpp_skip_snl_trailing`
- 外部函数：`cmpp_skip_space`
- 外部函数：`cmpp_sp_begin`
- 外部函数：`cmpp_sp_commit`
- 外部函数：`cmpp_sp_rollback`
- 外部函数：`cmpp_sqlite3_str_finish`
- 外部函数：`cmpp_stmt_bind_int`
- 外部函数：`cmpp_stmt_bind_int_text`
- 外部函数：`cmpp_stmt_bind_null`
- 外部函数：`cmpp_stmt_bind_textn`
- 外部函数：`cmpp_stmt_bind_textx`
- 外部函数：`cmpp_stmt_reset`
- 外部函数：`cmpp_stmt_step`
- 外部函数：`cmpp_stream`
- 外部函数：`cmpp_truthy`
- 外部函数：`cmpp_tt_cstr`
- 外部函数：`cmpp_tt_map`
- 外部函数：`cmpp_undefine`
- 外部函数：`cmpp_unpol_set`
- 外部函数：`coalesce`
- 外部函数：`def`
- 外部函数：`dxserr`
- 外部函数：`exit`
- 外部函数：`f`
- 外部函数：`fails`
- 外部函数：`fclose`
- 外部函数：`feof`
- 外部函数：`fflush`
- 外部函数：`fopen`
- 外部函数：`fprintf`
- 外部函数：`fputc`
- 外部函数：`fread`
- 外部函数：`g_debug`
- 外部函数：`g_warn`
- 外部函数：`g_warn0`
- 外部函数：`generate_series`
- 外部函数：`incl`
- 外部函数：`inclpath`
- 外部函数：`lout`
- 外部函数：`memcmp`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`modpath`
- 外部函数：`popcheck`
- 外部函数：`predef`
- 外部函数：`preprocessor`
- 外部函数：`printf`
- 外部函数：`recently`
- 外部函数：`replace`
- 外部函数：`s`
- 外部函数：`sdef`
- 外部函数：`serr`
- 外部函数：`snprintf`
- 外部函数：`sqlite3_busy_timeout`
- 外部函数：`sqlite3_close`
- 外部函数：`sqlite3_column_blob`
- 外部函数：`sqlite3_column_bytes`
- 外部函数：`sqlite3_column_int`
- 外部函数：`sqlite3_column_text`
- 外部函数：`sqlite3_create_function`
- 外部函数：`sqlite3_create_module`
- 外部函数：`sqlite3_db_config`
- 外部函数：`sqlite3_errcode`
- 外部函数：`sqlite3_exec`
- 外部函数：`sqlite3_expanded_sql`
- 外部函数：`sqlite3_finalize`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_libversion_number`
- 外部函数：`sqlite3_mprintf`
- 外部函数：`sqlite3_open_v2`
- 外部函数：`sqlite3_prepare_v2`
- 外部函数：`sqlite3_result_int`
- 外部函数：`sqlite3_result_int64`
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
- 外部函数：`treat`
- 外部函数：`ttype`
- 外部函数：`type`
- 外部函数：`udf_compare`
- 外部函数：`use`
- 外部函数：`using`
- 外部函数：`ustr_c`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 外部函数：`vdef`
- 外部函数：`vfprintf`
- 外部函数：`void`
- 外部函数：`xCall`
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
- 大写宏：`CMPP_CTOR_INSTANCE_INIT`
- 大写宏：`CMPP_MAIN_AUTOLOADER`
- 大写宏：`CMPP_OMIT_D_DB`
- 大写宏：`CMPP_OMIT_D_INCLUDE`
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
- 大写宏：`COMMIT`
- 大写宏：`CONFLICT`
- 大写宏：`CREATE`
- 大写宏：`DEFAULT`
- 大写宏：`DELETE`
- 大写宏：`DESC`
- 大写宏：`DETACH`
- 大写宏：`DROP`
- 大写宏：`DTRT`
- 大写宏：`END`
- 大写宏：`EOF`
- 大写宏：`EXCLUDED`
- 大写宏：`EXCLUSIVE`
- 大写宏：`EXISTS`
- 大写宏：`FAIL`
- 大写宏：`FALLTHRU`
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
- 大写宏：`MSG`
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
- 大写宏：`SQLITE_EXTENSION_INIT2`
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
- 大写宏：`SQLITE_STATIC`
- 大写宏：`SQLITE_TEXT`
- 大写宏：`SQLITE_TOOBIG`
- 大写宏：`SQLITE_TRACE_STMT`
- 大写宏：`SQLITE_UTF8`
- 大写宏：`START`
- 大写宏：`STR`
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
- 大写宏：`WHERE`
- 大写宏：`WHICH`
- 大写宏：`WITHOUT`
- 大写宏：`ZMSG`
- 外部类型：`APIs`
- 外部类型：`Add`
- 外部类型：`An`
- 外部类型：`Anything`
- 外部类型：`Arguable`
- 外部类型：`Assertion`
- 外部类型：`Backtick`
- 外部类型：`Basic`
- 外部类型：`Because`
- 外部类型：`Blob`
- 外部类型：`Cannot`
- 外部类型：`Chained`
- 外部类型：`CmppArgList`
- 外部类型：`CmppDLine`
- 外部类型：`CmppDList`
- 外部类型：`CmppDList_entry`
- 外部类型：`CmppDList_entry_cmp_pp`
- 外部类型：`CmppIfState`
- 外部类型：`CmppLvl`
- 外部类型：`CmppLvlList`
- 外部类型：`CmppLvl_F_ELIDE`
- 外部类型：`CmppLvl_F_INHERIT_MASK`
- 外部类型：`CmppLvl_empty`
- 外部类型：`CmppStmt`
- 外部类型：`CmppStmt_`
- 外部类型：`CmppStmt_cmpVD`
- 外部类型：`CmppStmt_cmpVV`
- 外部类型：`CmppStmt_defGet`
- 外部类型：`CmppStmt_defGetBool`
- 外部类型：`CmppStmt_defGetInt`
- 外部类型：`CmppStmt_defHas`
- 外部类型：`CmppStmt_defIns`
- 外部类型：`CmppStmt_defSelAll`
- 外部类型：`CmppStmt_e`
- 外部类型：`CmppStmt_insTtype`
- 外部类型：`CmppStmt_none`
- 外部类型：`CmppStmt_spBegin`
- 外部类型：`CmppStmt_spRelease`
- 外部类型：`CmppStmt_spRollback`
- 外部类型：`Compare`
- 外部类型：`Consume`
- 外部类型：`Current`
- 外部类型：`Decimal`
- 外部类型：`Decode`
- 外部类型：`Directives`
- 外部类型：`Do`
- 外部类型：`Drop`
- 外部类型：`Dsrcdir`
- 外部类型：`Emitting`
- 外部类型：`Empty`
- 外部类型：`Eq`
- 外部类型：`Error`
- 外部类型：`Expecting`
- 外部类型：`Expressions`
- 外部类型：`Extra`
- 外部类型：`Fall`
- 外部类型：`Ge`
- 外部类型：`Gt`
- 外部类型：`Heredoc`
- 外部类型：`Idir`
- 外部类型：`If`
- 外部类型：`Ill`
- 外部类型：`Illegal`
- 外部类型：`In`
- 外部类型：`Initialization`
- 外部类型：`Input`
- 外部类型：`Insert`
- 外部类型：`Installing`
- 外部类型：`Invalid`
- 外部类型：`Invalidates`
- 外部类型：`It`
- 外部类型：`Keep`
- 外部类型：`Key`
- 外部类型：`LFs`
- 外部类型：`Le`
- 外部类型：`Look`
- 外部类型：`Lt`
- 外部类型：`Maintenance`
- 外部类型：`Maybe`
- 外部类型：`Missing`
- 外部类型：`Misuse`
- 外部类型：`Neq`
- 外部类型：`No`
- 外部类型：`None`
- 外部类型：`Not`
- 外部类型：`Offending`
- 外部类型：`Once`
- 外部类型：`Only`
- 外部类型：`PRIu32`
- 外部类型：`Pass`
- 外部类型：`PodList__atpol`
- 外部类型：`Propagate`
- 外部类型：`Remember`
- 外部类型：`Reminder`
- 外部类型：`Resolving`
- 外部类型：`SQLite`
- 外部类型：`Schema`
- 外部类型：`See`
- 外部类型：`Similarly`
- 外部类型：`String`
- 外部类型：`StringBT`
- 外部类型：`StringDQ`
- 外部类型：`StringSQ`
- 外部类型：`Strip`
- 外部类型：`Such`
- 外部类型：`Symbolic`
- 外部类型：`Synthesize`
- 外部类型：`That`
- 外部类型：`The`
- 外部类型：`Then`
- 外部类型：`This`
- 外部类型：`To`
- 外部类型：`Too`
- 外部类型：`Transfer`
- 外部类型：`Treat`
- 外部类型：`Try`
- 外部类型：`Typically`
- 外部类型：`UDFs`
- 外部类型：`Unexpected`
- 外部类型：`Unhandled`
- 外部类型：`Unknown`
- 外部类型：`Unused`
- 外部类型：`Usage`
- 外部类型：`We`
- 外部类型：`Whole`
- 外部类型：`Word`
- 外部类型：`cmpp`
- 外部类型：`cmpp__delim`
- 外部类型：`cmpp__dx_pimpl`
- 外部类型：`cmpp__pimpl`
- 外部类型：`cmpp_api_thunk`
- 外部类型：`cmpp_arg`
- 外部类型：`cmpp_argOp`
- 外部类型：`cmpp_args`
- 外部类型：`cmpp_args_pimpl`
- 外部类型：`cmpp_atpol_e`
- 外部类型：`cmpp_b`
- 外部类型：`cmpp_d`
- 外部类型：`cmpp_d_autoloader`
- 外部类型：`cmpp_d_reg`
- 外部类型：`cmpp_dx`
- 外部类型：`cmpp_errinfo`
- 外部类型：`cmpp_f_args`
- 外部类型：`cmpp_outputer`
- 外部类型：`cmpp_tizer`
- 外部类型：`cmpp_token`
- 外部类型：`cmpp_tt_e`
- 外部类型：`cmpp_unpol_e`
- 外部类型：`int32_t`
- 外部类型：`int64_t`
- 外部类型：`size_t`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-da29976dc8` → 本草稿移入 `cases/defect/auto-sqlite-da29976dc8/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
