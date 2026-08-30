# auto-redis-e52672df58

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #14330 (https://github.com/redis/redis/pull/14330)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -809,6 +809,30 @@ dictEntry *dictFind(dict *d, const void *key)
     return (link) ? *link : NULL;
 }
 
+/* Finds the dictEntry using pointer and pre-calculated hash.
+ * oldkey is a dead pointer and should not be accessed.
+ * the hash value should be provided using dictGetHash.
+ * no string / key comparison is performed.
+ * return value is a pointer to the dictEntry if found, or NULL if not found. */
+dictEntry *dictFindByHashAndPtr(dict *d, const void *oldptr, const uint64_t hash) {
+    dictEntry *he;
+    unsigned long idx, table;
+
+    if (dictSize(d) == 0) return NULL; /* dict is empty */
+    for (table = 0; table <= 1; table++) {
+        idx = hash & DICTHT_SIZE_MASK(d->ht_size_exp[table]);
+        if (table == 0 && (long)idx < d->rehashidx) continue;
+        he = d->ht_table[table][idx];
+        while(he) {
+            if (oldptr == dictGetKey(he))
+                return he;
+            he = dictGetNext(he);
+        }
+        if (!dictIsRehashing(d)) return NULL;
+    }
+    return NULL;
+}
+
 /* Find a key and return its dictEntryLink reference. Otherwise, return NULL
  * 
  * A dictEntryLink pointer being used to find preceding dictEntry of searched item. 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-e52672df58` → 本草稿移入 `cases/defect/auto-redis-e52672df58/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
