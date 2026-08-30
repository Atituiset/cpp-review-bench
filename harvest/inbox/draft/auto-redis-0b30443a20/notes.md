# auto-redis-0b30443a20

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15489 (https://github.com/redis/redis/pull/15489)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 3813；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -3810,8 +3810,33 @@ void xsetidCommand(client *c) {
     }
 
     s->last_id = id;
-    if (entries_added != -1)
+    if (entries_added != -1) {
+        uint64_t prev_entries_added = s->entries_added;
         s->entries_added = entries_added;
+        /* Lowering entries_added may leave a consumer group's entries_read
+         * greater than the stream's entries_added. That breaks the lag
+         * calculation (XINFO GROUPS would report a negative lag) and, on
+         * builds that validate this invariant, produces an RDB/RESTORE
+         * payload that fails to load. Clamp each group's counter down, just
+         * like XGROUP CREATE/SETID does when entries_read is set too high.
+         * Only needed when entries_added is actually lowered; otherwise the
+         * entries_read <= entries_added invariant already holds and the loop
+         * would be a no-op. */
+        if (s->entries_added < prev_entries_added && s->cgroups) {
+            raxIterator ri;
+            raxStart(&ri, s->cgroups);
+            raxSeek(&ri, "^", NULL, 0);
+            while (raxNext(&ri)) {
+                streamCG *cg = ri.data;
+                if (cg->entries_read != SCG_INVALID_ENTRIES_READ &&
+                    (uint64_t)cg->entries_read > s->entries_added)
+                {
+                    cg->entries_read = s->entries_added;
+                }
+            }
+            raxStop(&ri);
+        }
+    }
     if (!streamIDEqZero(&max_xdel_id))
         s->max_deleted_entry_id = max_xdel_id;
     addReply(c,shared.ok);
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-0b30443a20` → 本草稿移入 `cases/defect/auto-redis-0b30443a20/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
