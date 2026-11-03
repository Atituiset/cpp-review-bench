# auto-postgres-3d1ba76508

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#3ba886b35a2804dcd275868a19135a31ceb81386](https://github.com/postgres/postgres/commit/3ba886b35a2804dcd275868a19135a31ceb81386) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-03 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 4 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #3ba886b35a2804dcd275868a19135a31ceb81386 (https://github.com/postgres/postgres/commit/3ba886b35a2804dcd275868a19135a31ceb81386)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 211；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 3ba886b35a2804dcd275868a19135a31ceb81386 Clean up code comments for pg_wchar_tbl :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -208,35 +208,115 @@ extern PGDLLIMPORT const char *pg_enc2gettext_tbl[];
 /*
  * pg_wchar stuff
  */
-typedef int (*mb2wchar_with_len_converter) (const unsigned char *from,
-											pg_wchar *to,
-											int len);
 
-typedef int (*wchar2mb_with_len_converter) (const pg_wchar *from,
-											unsigned char *to,
-											int len);
+typedef int (*mblen_function) (const unsigned char *mbstr);
+typedef int (*mbverifychar_function) (const unsigned char *mbstr, int len);
 
-typedef int (*mblen_converter) (const unsigned char *mbstr);
+/*
+ * Operations on multi-byte encodings are driven by a table of helper
+ * functions.
+ *
+ * All functions are required, except the conversion functions
+ * .mb2wchar_with_len and .wchar2mb_with_len, which are only required for
+ * server encodings.
+ *
+ * These functions generally assume that their input is validly formed, and
+ * unless specified otherwise have no special return value for errors.  The
+ * exceptions are the two "verify" functions, which must check their input and
+ * signal invalid input through their return value.  None of these functions
+ * report errors via ereport().
+ */
+typedef struct
+{
+	/*
+	 * Convert a multibyte string to a pg_wchar string.
+	 *
+	 * "len" is the length of "from" in bytes (not characters).  The input
+	 * string need not be zero-terminated, but conversion will stop early on a
+	 * zero byte.  The output is written to "to".  "to" must be allocated to
+	 * accommodate at least len + 1 pg_wchar's.  A terminating 0 is always
+	 * written.  The return value is the number of pg_wchar's written, without
+	 * the terminator.
+	 *
+	 * If a multibyte sequence at the end is cut off by len, it is silently
+	 * dropped.
+	 */
+	int			(*mb2wchar_with_len) (const unsigned char *from, pg_wchar *to, int len);
 
-typedef int (*mbdisplaylen_converter) (const unsigned char *mbstr);
+	/*
+	 * Convert a pg_wchar string to a multibyte string.
+	 *
+	 * "len" is the length of "from" in pg_wchar characters (not bytes).  The
+	 * input need not be terminated by a 0 pg_wchar, but conversion will stop
+	 * early on a 0 pg_wchar.  The output is written to "to".  "to" must be
+	 * allocated as at least len * maxmblen + 1 bytes.  A terminating 0 byte
+	 * is always written.  The return value is the number of bytes written,
+	 * without the terminator.
+	 */
+	int			(*wchar2mb_with_len) (const pg_wchar *from, unsigned char *to, int len);
 
-typedef bool (*mbcharacter_incrementer) (unsigned char *mbstr, int len);
+	/*
+	 * Get byte length of the first character of the input string.
+	 *
+	 * If the input is an empty string (*mbstr == '\0'), then 1 is returned.
+	 * The return value is always between 1 and maxmblen inclusive.
+	 *
+	 * We expect that mblen does not need to examine more than the first byte
+	 * of the character to discover the correct length.  GB18030 is an
+	 * exception; see pg_gb18030_mblen().
+	 */
+	mblen_function mblen;
 
-typedef int (*mbchar_verifier) (const unsigned char *mbstr, int len);
+	/*
+	 * Get the display width of the first character of the input string.
+	 *
+	 * The possible results are -1, 0, 1 and 2.  Control characters return -1;
+	 * callers can handle that specially.  Unlike mblen, an empty string
+	 * returns 0.  (But the converse is not true: Zero-width characters also
+	 * return 0.)
+	 *
+	 * Note: for the display output of psql to work properly, the return
+	 * values of the dsplen functions must conform to the Unicode standard. In
+	 * particular the NUL character is zero width and control characters are
+	 * generally width -1. It is recommended that non-ASCII encodings refer
+	 * their ASCII subset to the ASCII routines to ensure consistency.
+	 */
+	int			(*dsplen) (const unsigned char *mbstr);
 
-typedef int (*mbstr_verifier) (const unsigned char *mbstr, int len);
+	/*
+	 * Verify multibyte character.
+	 *
+	 * The verifychar functions accept "mbstr", a pointer to the first byte of
+	 * a string, and "len", the remain
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bool`
- 外部函数：`int`
- 外部函数：`pg_database_encoding_character_incrementer`
- 外部函数：`pg_database_encoding_max_length`
- 外部函数：`pg_mbcharcliplen`
- 大写宏：`PGDLLIMPORT`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-postgres-3d1ba76508` → 本草稿移入 `cases/defect/auto-postgres-3d1ba76508/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
