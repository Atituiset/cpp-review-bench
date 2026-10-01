# auto-redis-3502ef6346

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #14789 (https://github.com/redis/redis/pull/14789)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 388；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -385,9 +385,10 @@ unsigned long long kvstoreScan(kvstore *kvs, unsigned long long cursor,
  */
 int kvstoreExpand(kvstore *kvs, uint64_t newsize, int try_expand, kvstoreExpandShouldSkipDictIndex *skip_cb) {
     for (int i = 0; i < kvs->num_dicts; i++) {
-        dict *d = kvstoreGetDict(kvs, i);
-        if (!d || (skip_cb && skip_cb(i)))
-            continue;
+        if (skip_cb && skip_cb(i)) continue;
+        dict *d = createDictIfNeeded(kvs, i);
+        if (!d) continue;
+
         int result = try_expand ? dictTryExpand(d, newsize) : dictExpand(d, newsize);
         if (try_expand && result == DICT_ERR)
             return 0;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-3502ef6346` → 本草稿移入 `cases/defect/auto-redis-3502ef6346/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
