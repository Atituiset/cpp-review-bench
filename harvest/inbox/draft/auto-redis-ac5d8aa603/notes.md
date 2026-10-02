# auto-redis-ac5d8aa603

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | redis/redis |
| 源 PR | [#15804](https://github.com/redis/redis/pull/15804) |
| 许可证 | RSALv2 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 37 |
| 编译错误数（gcc syntax-only） | 23（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #15804 (https://github.com/redis/redis/pull/15804)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 43；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 15804 Redis 8.10.2 :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -38,9 +38,16 @@
 #include <ctype.h>
 #include <string.h>
 
+typedef unsigned short recursion_depth_t;
+
+// Don't see much point in making the depth configurable by the caller
+// Making it hardcoded to simplify the callers usage
+// Used to avoid getting into a stack overflow in case of malicious json input
+#define MAX_RECURSION_DEPTH_LIMIT 1000
+
 // Forward declarations.
 static int jsonSkipValue(const char **p, const char *end);
-static exprtoken *jsonParseValueToken(const char **p, const char *end);
+static exprtoken *jsonParseValueToken(const char **p, const char *end, recursion_depth_t current);
 
 /* Similar to ctype.h isdigit() but covers the whole JSON number charset,
  * including exp form. */
@@ -267,7 +274,7 @@ static exprtoken *jsonParseLiteralToken(const char **p, const char *end, const c
     return t;
 }
 
-static exprtoken *jsonParseArrayToken(const char **p, const char *end) {
+static exprtoken *jsonParseArrayToken(const char **p, const char *end, recursion_depth_t current) {
     if (*p >= end || **p != '[') return NULL;
     (*p)++; // Skip '['.
     jsonSkipWhiteSpaces(p,end);
@@ -283,7 +290,7 @@ static exprtoken *jsonParseArrayToken(const char **p, const char *end) {
 
     // Parse array elements.
     while (1) {
-        exprtoken *ele = jsonParseValueToken(p,end);
+        exprtoken *ele = jsonParseValueToken(p,end, current + 1);
         if (!ele) {
             exprTokenRelease(t); // Clean up partially built array token.
             return NULL;
@@ -330,13 +337,17 @@ static exprtoken *jsonParseArrayToken(const char **p, const char *end) {
 }
 
 /* Turn a JSON value into an expr token. */
-static exprtoken *jsonParseValueToken(const char **p, const char *end) {
+static exprtoken *jsonParseValueToken(const char **p, const char *end, recursion_depth_t current) {
+    if (current >= MAX_RECURSION_DEPTH_LIMIT) {
+        // protect from stack overflow
+        return NULL; 
+    }
     jsonSkipWhiteSpaces(p,end);
     if (*p >= end) return NULL;
 
     switch (**p) {
     case '"': return jsonParseStringToken(p,end);
-    case '[':  return jsonParseArrayToken(p,end);
+    case '[':  return jsonParseArrayToken(p,end,current);
     case '{':  return NULL; // No nested elements support for now.
     case 't':  return jsonParseLiteralToken(p,end,"true",EXPR_TOKEN_NUM,1);
     case 'f':  return jsonParseLiteralToken(p,end,"false",EXPR_TOKEN_NUM,0);
@@ -437,5 +448,5 @@ exprtoken *jsonExtractField(const char *json, size_t json_len,
 
     /* Key found, valptr points to the start of the value.
      * Convert it into an expression token object. */
-    return jsonParseValueToken(&valptr,end);
+    return jsonParseValueToken(&valptr,end, 0);
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`exprNewToken`
- 外部函数：`failed`
- 外部函数：`isspace`
- 外部函数：`jsonIsNumberChar`
- 外部函数：`jsonParseValueToken`
- 外部函数：`skipping`
- 外部函数：`strlen`
- 外部函数：`strncmp`
- 大写宏：`EXPR_TOKEN_STR`
- 大写宏：`JSON`
- 大写宏：`NULL`
- 外部类型：`Advance`
- 外部类型：`Always`
- 外部类型：`Any`
- 外部类型：`Check`
- 外部类型：`Continue`
- 外部类型：`Ensure`
- 外部类型：`Escapes`
- 外部类型：`Forward`
- 外部类型：`Found`
- 外部类型：`Goal`
- 外部类型：`If`
- 外部类型：`Literal`
- 外部类型：`Loop`
- 外部类型：`No`
- 外部类型：`Null`
- 外部类型：`Otherwise`
- 外部类型：`Return`
- 外部类型：`Similar`
- 外部类型：`Skip`
- 外部类型：`String`
- 外部类型：`Supported`
- 外部类型：`This`
- 外部类型：`Unterminated`
- 外部类型：`size_t`

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

1. 完成上面检查清单后评论 `/case accept auto-redis-ac5d8aa603` → 本草稿移入 `cases/defect/auto-redis-ac5d8aa603/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
