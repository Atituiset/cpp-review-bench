# auto-redis-25158427e6

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | redis/redis |
| 源 PR | [#16034](https://github.com/redis/redis/pull/16034) |
| 许可证 | RSALv2 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-23 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 53 |
| 编译错误数（gcc syntax-only） | 14（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #16034 (https://github.com/redis/redis/pull/16034)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 8（原始 PR diff 行 547；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 16034 Redis cli cmddocs hardening :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -544,7 +544,7 @@ static void cliAddCommandDocArg(cliCommandArg *cmdArg, redisReply *argMap) {
         return;
     }
 
-    for (size_t i = 0; i < argMap->elements; i += 2) {
+    for (size_t i = 0; i + 1 < argMap->elements; i += 2) {
         assert(argMap->element[i]->type == REDIS_REPLY_STRING);
         char *key = argMap->element[i]->str;
         if (!strcmp(key, "name")) {
@@ -641,11 +641,17 @@ static void cliFillInCommandHelpEntry(helpEntry *help, char *cmdname, char *subc
 static helpEntry *cliInitCommandHelpEntry(char *cmdname, char *subcommandname,
                                           helpEntry *next, redisReply *specs,
                                           dict *groups) {
+    /* Defensive bound: the reply is untrusted, and if for any reason it yields
+     * more entries than cliCountCommands() allocated, stop instead of writing
+     * past the end of the helpEntries array. helpEntriesLen still holds the
+     * allocated count here; cliInitCommandHelpEntries() trims it only after
+     * the fill. */
+    if (next >= helpEntries + helpEntriesLen) return next;
     helpEntry *help = next++;
     cliFillInCommandHelpEntry(help, cmdname, subcommandname);
 
     assert(specs->type == REDIS_REPLY_MAP || specs->type == REDIS_REPLY_ARRAY);
-    for (size_t j = 0; j < specs->elements; j += 2) {
+    for (size_t j = 0; j + 1 < specs->elements; j += 2) {
         assert(specs->element[j]->type == REDIS_REPLY_STRING);
         char *key = specs->element[j]->str;
         if (!strcmp(key, "summary")) {
@@ -671,10 +677,14 @@ static helpEntry *cliInitCommandHelpEntry(char *cmdname, char *subcommandname,
             help->docs.numargs = arguments->elements;
             cliMakeCommandDocArgs(arguments, help->docs.args);
             help->docs.params = makeHint(NULL, 0, 0, help->docs);
-        } else if (!strcmp(key, "subcommands")) {
+        } else if (!strcmp(key, "subcommands") && subcommandname == NULL) {
+            /* Only top-level commands may have subcommands. Redis has no nested
+             * subcommands, and cliCountCommands() only reserves space for one
+             * level, so ignore any deeper nesting a malicious server might send.
+             * Together with the count that keeps allocation and fill in lockstep. */
             redisReply *subcommands = specs->element[j + 1];
             assert(subcommands->type == REDIS_REPLY_MAP || subcommands->type == REDIS_REPLY_ARRAY);
-            for (size_t i = 0; i < subcommands->elements; i += 2) {
+            for (size_t i = 0; i + 1 < subcommands->elements; i += 2) {
                 assert(subcommands->element[i]->type == REDIS_REPLY_STRING);
                 char *subcommandname = subcommands->element[i]->str;
                 redisReply *subcommand = subcommands->element[i + 1];
@@ -690,13 +700,18 @@ static helpEntry *cliInitCommandHelpEntry(char *cmdname, char *subcommandname,
 static size_t cliCountCommands(redisReply* commandTable) {
     size_t numCommands = commandTable->elements / 2;
 
-    /* The command docs table maps command names to a map of their specs. */    
-    for (size_t i = 0; i < commandTable->elements; i += 2) {
+    /* The command docs table maps command names to a map of their specs.
+     * The reply comes from the server and may be malformed, so every paired
+     * access is guarded with `i + 1 < elements` to avoid reading past the
+     * element array on an odd-length reply. Only one level of subcommands is
+     * counted here; that must match the fill side in
+     * cliInitCommandHelpEntry(), which refuses to recurse deeper. */
+    for (size_t i = 0; i + 1 < commandTable->elements; i += 2) {
         assert(commandTable->element[i]->type == REDIS_REPLY_STRING);  /* Command name. */
         assert(commandTable->element[i + 1]->type == REDIS_REPLY_MAP ||
                commandTable->element[i + 1]->type == REDIS_REPLY_ARRAY);
         redisReply *map = commandTable->element[i + 1];
-        for (size_t j = 0
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`makeHint`
- 外部函数：`sdscat`
- 外部函数：`sdsnew`
- 外部函数：`sdstoupper`
- 外部函数：`strchr`
- 外部函数：`strcmp`
- 外部函数：`zcalloc`
- 外部函数：`zmalloc`
- 大写宏：`ARG_TYPE_BLOCK`
- 大写宏：`ARG_TYPE_DOUBLE`
- 大写宏：`ARG_TYPE_INTEGER`
- 大写宏：`ARG_TYPE_KEY`
- 大写宏：`ARG_TYPE_ONEOF`
- 大写宏：`ARG_TYPE_PATTERN`
- 大写宏：`ARG_TYPE_PURE_TOKEN`
- 大写宏：`ARG_TYPE_STRING`
- 大写宏：`ARG_TYPE_UNIX_TIME`
- 大写宏：`CMD_ARG_MULTIPLE`
- 大写宏：`CMD_ARG_MULTIPLE_TOKEN`
- 大写宏：`CMD_ARG_OPTIONAL`
- 大写宏：`NULL`
- 大写宏：`REDIS_REPLY_ARRAY`
- 大写宏：`REDIS_REPLY_MAP`
- 大写宏：`REDIS_REPLY_SET`
- 大写宏：`REDIS_REPLY_STATUS`
- 大写宏：`REDIS_REPLY_STRING`
- 外部类型：`Only`
- 外部类型：`Subcommand`
- 外部类型：`commandDocs`
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

1. 完成上面检查清单后评论 `/case accept auto-redis-25158427e6` → 本草稿移入 `cases/defect/auto-redis-25158427e6/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
