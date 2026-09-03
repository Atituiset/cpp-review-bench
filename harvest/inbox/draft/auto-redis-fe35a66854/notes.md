# auto-redis-fe35a66854

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #14979 (https://github.com/redis/redis/pull/14979)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 10（原始 PR diff 行 1687；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1657,6 +1657,10 @@ ssize_t rdbSaveDb(rio *rdb, int dbid, int rdbflags, long *key_counter, unsigned
             written += res;
             if ((res = rdbSaveLen(rdb, kvstoreDictSize(db->expires, curr_slot))) < 0) goto werr2;
             written += res;
+            /* Dismiss bucket arrays of the previous slot to reduce CoW.
+             * The final slot is not dismissed since the child exits shortly after. */
+            if (server.in_fork_child && last_slot != -1)
+                dismissDictBucketsMemory(kvstoreGetDict(db->keys, last_slot));
             last_slot = curr_slot;
         }
         kvobj *kv = dictGetKV(de);
@@ -1684,7 +1688,8 @@ ssize_t rdbSaveDb(rio *rdb, int dbid, int rdbflags, long *key_counter, unsigned
          * OS and possibly avoid or decrease COW. We give the dismiss
          * mechanism a hint about an estimated size of the object we stored. */
         size_t dump_size = rdb->processed_bytes - rdb_bytes_before_key;
-        if (server.in_fork_child) dismissObject(kv, dump_size);
+        if (server.in_fork_child && dump_size > server.page_size/2)
+            dismissObject(kv, dump_size);
 
         /* Update child info every 1 second (approximately).
          * in order to avoid calling mstime() on each iteration, we will
@@ -1735,6 +1740,10 @@ int rdbSaveRio(int req, rio *rdb, int *error, int rdbflags, rdbSaveInfo *rsi) {
     if (!(req & SLAVE_REQ_RDB_EXCLUDE_DATA)) {
         for (j = 0; j < server.dbnum; j++) {
             if (rdbSaveDb(rdb, j, rdbflags, &key_counter, &skipped) == -1) goto werr;
+            /* In standalone mode, dismiss bucket arrays of the saved DB's
+             * kvstore to reduce CoW. In cluster mode this is done per-slot. */
+            if (server.in_fork_child && !server.cluster_enabled)
+                dismissKvstoreBucketsMemory(server.db[j].keys);
         }
     }
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-fe35a66854` → 本草稿移入 `cases/defect/auto-redis-fe35a66854/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
