# auto-redis-e0128431e9

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15055 (https://github.com/redis/redis/pull/15055)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -871,7 +871,15 @@ typedef enum {
  * - debug.c - xorObjectDigest, serverLogObjectDebugInfo
  * - defrag.c - defragKey
  * - module.c - RM_KeyType (and add the new keytype to redismodule.h)
- * - object.c - object(create/free/dismiss/allocSize/Length) */
+ * - object.c - object(create/free/dismiss/allocSize/Length)
+ * - tests/support/util.tcl:generate_fuzzy_traffic_on_key - add command(s) for the new object type to the `commands` dict.
+ *
+ * If the new object type requires new command group make sure to update the following:
+ * - src/commands/command-docs.json - update the group:oneOf map with the new group
+ * - utils/generate-command-code.py - add the new group to GROUPS and COMMAND_GROUP_STR arrays
+ * - src/acl.c - add the new group to ACLDefaultCommandCategories array
+ * - src/server.h - add the new group to redisCommandGroup enum
+ * - if needed add new KSN type related to the group - search for NOTIFY_* and REDISMODULE_NOTIFY_* defines. */
 
 /* Extract encver / signature from a module type ID. */
 #define REDISMODULE_TYPE_ENCVER_BITS 10
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-e0128431e9` → 本草稿移入 `cases/defect/auto-redis-e0128431e9/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
