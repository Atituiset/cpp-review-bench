# auto-redis-7e3c26e27c

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #14700 (https://github.com/redis/redis/pull/14700)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 787；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -783,17 +783,11 @@ static dictEntryLink dictFindLinkInternal(dict *d, const void *key, dictEntryLin
         if (table == 0 && (long)idx < d->rehashidx) continue;
         idx = hash & DICTHT_SIZE_MASK(d->ht_size_exp[table]);
 
-        /* Prefetch the bucket at the calculated index */
-        redis_prefetch_read(&d->ht_table[table][idx]);
-
         link = &(d->ht_table[table][idx]);
         if (bucket) *bucket = link;
         while(link && *link) {
             const void *visitedKey = dictStoredKey2Key(d, dictGetKey(*link));
 
-            /* Prefetch the next entry to improve cache efficiency */
-            redis_prefetch_read(dictGetNext(*link));
-
             if (key == visitedKey || cmpFunc( &cmpCache, key, visitedKey))                
                 return link;
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-7e3c26e27c` → 本草稿移入 `cases/defect/auto-redis-7e3c26e27c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
