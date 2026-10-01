# auto-redis-fd3251489a

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #14968 (https://github.com/redis/redis/pull/14968)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -436,6 +436,8 @@ void debugCommand(client *c) {
 "    Show low level info about `key` and associated value.",
 "DROP-CLUSTER-PACKET-FILTER <packet-type>",
 "    Drop all packets that match the filtered type. Set to -1 allow all packets.",
+"ENABLE-KEYMETA-RUNTIME-REGISTRATION <0|1>",
+"    Allow keymeta class registration outside server startup (for testing).",
 "OOM",
 "    Crash the server simulating an out-of-memory error.",
 "PANIC",
@@ -927,6 +929,11 @@ NULL
     {
         server.skip_checksum_validation = atoi(c->argv[2]->ptr);
         addReply(c,shared.ok);
+    } else if (!strcasecmp(c->argv[1]->ptr,"enable-keymeta-runtime-registration") &&
+               c->argc == 3)
+    {
+        server.allow_keymeta_registration = atoi(c->argv[2]->ptr);
+        addReply(c,shared.ok);
     } else if (!strcasecmp(c->argv[1]->ptr,"aof-flush-sleep") &&
                c->argc == 3)
     {
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-fd3251489a` → 本草稿移入 `cases/defect/auto-redis-fd3251489a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
