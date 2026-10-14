# auto-sqlite-6e18b9c268

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#cf9555999e3da46f4f2934ddaf280b6d7e5c3bd7](https://github.com/sqlite/sqlite/commit/cf9555999e3da46f4f2934ddaf280b6d7e5c3bd7) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-14 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 14 |
| 编译错误数（gcc syntax-only） | 7（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #cf9555999e3da46f4f2934ddaf280b6d7e5c3bd7 (https://github.com/sqlite/sqlite/commit/cf9555999e3da46f4f2934ddaf280b6d7e5c3bd7)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 211；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT cf9555999e3da46f4f2934ddaf280b6d7e5c3bd7 Merge all recent branch-3.54 fixes into trunk :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -177,13 +177,10 @@ static int fts5AsciiTokenize(
 
 
 /*
-** The following two macros - READ_UTF8 and WRITE_UTF8 - have been copied
-** from the sqlite3 source file utf.c. If this file is compiled as part
-** of the amalgamation, they are not required.
+** The following two macros - FTS5_READ_UTF8 and FTS5_WRITE_UTF8 - have 
+** been copied from the sqlite3 source file utf.c. 
 */
-#ifndef SQLITE_AMALGAMATION
-
-static const unsigned char sqlite3Utf8Trans1[] = {
+static const unsigned char fts5Utf8Trans1[] = {
   0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
   0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
   0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
@@ -194,45 +191,43 @@ static const unsigned char sqlite3Utf8Trans1[] = {
   0x00, 0x01, 0x02, 0x03, 0x00, 0x01, 0x00, 0x00,
 };
 
-#define READ_UTF8(zIn, zTerm, c)                           \
+#define FTS5_READ_UTF8(zIn, zTerm, c)                      \
   c = *(zIn++);                                            \
   if( c>=0xc0 ){                                           \
-    c = sqlite3Utf8Trans1[c-0xc0];                         \
+    c = fts5Utf8Trans1[c-0xc0];                            \
     while( zIn<zTerm && (*zIn & 0xc0)==0x80 ){             \
       c = (c<<6) + (0x3f & *(zIn++));                      \
     }                                                      \
     if( c<0x80                                             \
-        || (c&0xFFFFF800)==0xD800                          \
-        || (c&0xFFFFFFFE)==0xFFFE ){  c = 0xFFFD; }        \
+     || (c&0xFFFFF800)==0xD800                             \
+     || (c&0xFFFFFFFE)==0xFFFE ){  c = 0xFFFD; }           \
   }
 
 
-#define WRITE_UTF8(zOut, c) {                          \
-  if( c<0x00080 ){                                     \
-    *zOut++ = (unsigned char)(c&0xFF);                 \
-  }                                                    \
-  else if( c<0x00800 ){                                \
-    *zOut++ = 0xC0 + (unsigned char)((c>>6)&0x1F);     \
-    *zOut++ = 0x80 + (unsigned char)(c & 0x3F);        \
-  }                                                    \
-  else if( c<0x10000 ){                                \
-    *zOut++ = 0xE0 + (unsigned char)((c>>12)&0x0F);    \
-    *zOut++ = 0x80 + (unsigned char)((c>>6) & 0x3F);   \
-    *zOut++ = 0x80 + (unsigned char)(c & 0x3F);        \
-  }else{                                               \
-    *zOut++ = 0xF0 + (unsigned char)((c>>18) & 0x07);  \
-    *zOut++ = 0x80 + (unsigned char)((c>>12) & 0x3F);  \
-    *zOut++ = 0x80 + (unsigned char)((c>>6) & 0x3F);   \
-    *zOut++ = 0x80 + (unsigned char)(c & 0x3F);        \
-  }                                                    \
+#define FTS5_WRITE_UTF8(zOut, c) {                         \
+  if( c<0x00080 ){                                         \
+    *zOut++ = (unsigned char)(c&0xFF);                     \
+  }                                                        \
+  else if( c<0x00800 ){                                    \
+    *zOut++ = 0xC0 + (unsigned char)((c>>6)&0x1F);         \
+    *zOut++ = 0x80 + (unsigned char)(c & 0x3F);            \
+  }                                                        \
+  else if( c<0x10000 ){                                    \
+    *zOut++ = 0xE0 + (unsigned char)((c>>12)&0x0F);        \
+    *zOut++ = 0x80 + (unsigned char)((c>>6) & 0x3F);       \
+    *zOut++ = 0x80 + (unsigned char)(c & 0x3F);            \
+  }else{                                                   \
+    *zOut++ = 0xF0 + (unsigned char)((c>>18) & 0x07);      \
+    *zOut++ = 0x80 + (unsigned char)((c>>12) & 0x3F);      \
+    *zOut++ = 0x80 + (unsigned char)((c>>6) & 0x3F);       \
+    *zOut++ = 0x80 + (unsigned char)(c & 0x3F);            \
+  }                                                        \
 }
 
-#endif /* ifndef SQLITE_AMALGAMATION */
-
-#define FTS5_SKIP_UTF8(zIn) {                               \
-  if( ((unsigned char)(*(zIn++)))>=0xc0 ){       
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`memmove`
- 外部函数：`sqlite3Fts5UnicodeCategory`
- 外部函数：`sqlite3Fts5UnicodeFold`
- 外部函数：`sqlite3Fts5UnicodeIsdiacritic`
- 大写宏：`SQLITE_AMALGAMATION`
- 大写宏：`SQLITE_OK`
- 外部类型：`An`
- 外部类型：`At`
- 外部类型：`Fold`
- 外部类型：`If`
- 外部类型：`Or`
- 外部类型：`Skip`
- 外部类型：`The`
- 外部类型：`Unicode61Tokenizer`
- 外部类型：`Update`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-6e18b9c268` → 本草稿移入 `cases/defect/auto-sqlite-6e18b9c268/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
