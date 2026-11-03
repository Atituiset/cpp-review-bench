# auto-postgres-1e036a868a

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
| 外部依赖数（dep_count） | 45 |
| 编译错误数（gcc syntax-only） | 8（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #3ba886b35a2804dcd275868a19135a31ceb81386 (https://github.com/postgres/postgres/commit/3ba886b35a2804dcd275868a19135a31ceb81386)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 3ba886b35a2804dcd275868a19135a31ceb81386 Clean up code comments for pg_wchar_tbl :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -37,32 +37,6 @@
 #define NONUTF8_INVALID_BYTE1 (' ')
 
 
-/*
- * Operations on multi-byte encodings are driven by a table of helper
- * functions.
- *
- * To add an encoding support, define mblen(), dsplen(), verifychar() and
- * verifystr() for the encoding.  For server-encodings, also define mb2wchar()
- * and wchar2mb() conversion functions.
- *
- * These functions generally assume that their input is validly formed.
- * The "verifier" functions, further down in the file, have to be more
- * paranoid.
- *
- * We expect that mblen() does not need to examine more than the first byte
- * of the character to discover the correct length.  GB18030 is an exception
- * to that rule, though, as it also looks at second byte.  But even that
- * behaves in a predictable way, if you only pass the first byte: it will
- * treat 4-byte encoded characters as two 2-byte encoded characters, which is
- * good enough for all current uses.
- *
- * Note: for the display output of psql to work properly, the return values
- * of the dsplen functions must conform to the Unicode standard. In particular
- * the NUL character is zero width and control characters are generally
- * width -1. It is recommended that non-ASCII encodings refer their ASCII
- * subset to the ASCII routines to ensure consistency.
- */
-
 /* No error-reporting facility.  Ignore incomplete trailing byte sequence. */
 #define MB2CHAR_NEED_AT_LEAST(len, need) if ((len) < (need)) break
 
@@ -390,9 +364,6 @@ pg_euctw_dsplen(const unsigned char *s)
 
 /*
  * Convert pg_wchar to EUC_* encoding.
- * caller must allocate enough space for "to", including a trailing zero!
- * len: length of from.
- * "from" not necessarily null terminated.
  */
 static int
 pg_wchar2euc_with_len(const pg_wchar *from, unsigned char *to, int len)
@@ -454,9 +425,6 @@ pg_johab_dsplen(const unsigned char *s)
 
 /*
  * convert UTF8 string to pg_wchar (UCS-4)
- * caller must allocate enough space for "to", including a trailing zero!
- * len: length of from.
- * "from" not necessarily null terminated.
  */
 static int
 pg_utf2wchar_with_len(const unsigned char *from, pg_wchar *to, int len)
@@ -517,9 +485,6 @@ pg_utf2wchar_with_len(const unsigned char *from, pg_wchar *to, int len)
 
 /*
  * Trivial conversion from pg_wchar to UTF-8.
- * caller should allocate enough space for "to"
- * len: length of from.
- * "from" not necessarily null terminated.
  */
 static int
 pg_wchar2utf_with_len(const pg_wchar *from, unsigned char *to, int len)
@@ -704,9 +669,6 @@ pg_latin12wchar_with_len(const unsigned char *from, pg_wchar *to, int len)
 /*
  * Trivial conversion from pg_wchar to single byte encoding. Just ignores
  * high bits.
- * caller should allocate enough space for "to"
- * len: length of from.
- * "from" not necessarily null terminated.
  */
 static int
 pg_wchar2single_with_len(const pg_wchar *from, unsigned char *to, int len)
@@ -891,25 +853,9 @@ pg_gb18030_dsplen(const unsigned char *s)
 /*
  *-------------------------------------------------------------------
  * multibyte sequence validators
- *
- * The verifychar functions accept "s", a pointer to the first byte of a
- * string, and "len", the remaining length of the string.  If there is a
- * validly encoded character beginning at *s, return its length in bytes;
- * else return -1.
- *
- * The verifystr functions also accept "s", a pointer to a string and "len",
- * the length of the string.  They verify the whole string, and return the
- * number of input bytes (<= len) that are valid.  In other words, if the
- * whole string is valid, verifystr returns "len", otherwise it returns the
- * byte offset of the first invalid character.  The verifystr functions must
- * test for and reject zeroes in the input.
- *
- * The verifychar functions can assume that len > 0 and that *s != '\0', but
- * they must test for and reject zeroes in any additional bytes of a
- * multibyte character.  Note that this definition allows the function for a
- * single-byte encodi
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`dsplen`
- 外部函数：`mb2wchar`
- 外部函数：`pg_ascii_verifychar`
- 外部函数：`pg_utf2wchar_with_len`
- 外部函数：`pg_wchar2euc_with_len`
- 外部函数：`unicode_to_utf8`
- 外部函数：`verifychar`
- 外部函数：`verifystr`
- 外部函数：`wchar2mb`
- 大写宏：`ASCII`
- 大写宏：`EUC_`
- 大写宏：`GB18030`
- 大写宏：`NOT_USED`
- 大写宏：`NUL`
- 大写宏：`UCS`
- 大写宏：`UTF`
- 大写宏：`UTF8`
- 外部类型：`But`
- 外部类型：`Convert`
- 外部类型：`For`
- 外部类型：`If`
- 外部类型：`Ignore`
- 外部类型：`In`
- 外部类型：`It`
- 外部类型：`Just`
- 外部类型：`No`
- 外部类型：`Note`
- 外部类型：`Operations`
- 外部类型：`The`
- 外部类型：`These`
- 外部类型：`They`
- 外部类型：`To`
- 外部类型：`Trivial`
- 外部类型：`Unicode`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-1e036a868a` → 本草稿移入 `cases/defect/auto-postgres-1e036a868a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
