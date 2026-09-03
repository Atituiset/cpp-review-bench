# auto-redis-8e432b4693

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15308 (https://github.com/redis/redis/pull/15308)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -3409,6 +3409,12 @@ robj *rdbLoadObject(int rdbtype, rio *rdb, sds key, int dbid, int *error)
             return NULL;
         }
 
+        if (s->entries_added > (uint64_t)LLONG_MAX || s->entries_added < s->length) {
+            rdbReportCorruptRDB("Stream entries_added inconsistent with length");
+            decrRefCount(o);
+            return NULL;
+        }
+
         /* Consumer groups loading */
         uint64_t cgroups_count = rdbLoadLen(rdb,NULL);
         if (cgroups_count == RDB_LENERR) {
@@ -3452,6 +3458,15 @@ robj *rdbLoadObject(int rdbtype, rio *rdb, sds key, int dbid, int *error)
                 cg_offset = streamEstimateDistanceFromFirstEverEntry(s,&cg_id);
             }
 
+            if ((int64_t)cg_offset != SCG_INVALID_ENTRIES_READ &&
+                (cg_offset > (uint64_t)LLONG_MAX || cg_offset > s->entries_added))
+            {
+                rdbReportCorruptRDB("Stream cgroup entries_read inconsistent with entries_added");
+                sdsfree(cgname);
+                decrRefCount(o);
+                return NULL;
+            }
+
             streamCG *cgroup = streamCreateCG(s,cgname,sdslen(cgname),&cg_id,cg_offset);
             if (cgroup == NULL) {
                 rdbReportCorruptRDB("Duplicated consumer group name %s",
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-8e432b4693` → 本草稿移入 `cases/defect/auto-redis-8e432b4693/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
