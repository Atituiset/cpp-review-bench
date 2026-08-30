# auto-redis-f54413e2f5

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15230 (https://github.com/redis/redis/pull/15230)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -2184,7 +2184,7 @@ size_t VectorSetMemUsage(const void *value) {
     /* Add the 0.33 remaining part, but upper layers have less links. */
     size += (sizeof(hnswNode*) * other_levels_links * vset->hnsw->node_count)/3;
 
-    /* Associated string value and attributres.
+    /* Associated string value and attributes.
      * Use Redis Module API to get string size, and guess that all the
      * elements have similar size as the first few. */
     size_t items_scanned = 0, items_size = 0;
@@ -2206,7 +2206,7 @@ size_t VectorSetMemUsage(const void *value) {
     if (items_scanned)
         size += items_size / items_scanned * vset->hnsw->node_count;
 
-    /* Add memory usage due to attributres. */
+    /* Add memory usage due to attributes. */
     if (attribs_scanned == 0) {
         /* We were not lucky enough to find a single attribute in the
          * first few items? Let's use a fixed arbitrary value. */
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-f54413e2f5` → 本草稿移入 `cases/defect/auto-redis-f54413e2f5/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
