# auto-redis-00f3f3c65f

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15569 (https://github.com/redis/redis/pull/15569)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 3265；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -42,7 +42,7 @@ int streamParseStrictIDOrReply(client *c, robj *o, streamID *id, uint64_t missin
 int streamParseIDOrReply(client *c, robj *o, streamID *id, uint64_t missing_seq);
 
 int streamEntryIsReferenced(stream *s, streamID *id);
-void streamCleanupEntryCGroupRefs(stream *s, streamID *id);
+int streamCleanupEntryCGroupRefs(stream *s, streamID *id);
 void streamUpdateCGroupLastId(stream *s, streamCG *cg, streamID *id);
 void trackStreamClaimTimeouts(client *c, robj **keys, int numkeys, uint64_t expire_time);
 
@@ -3260,9 +3260,10 @@ void streamUnlinkEntryFromCGroupRef(stream *s, streamNACK *na, unsigned char *ke
     }
 }
 
-/* Remove all consumer group references to a specific stream message. */
-void streamCleanupEntryCGroupRefs(stream *s, streamID *id) {
-    if (!s->cgroups_ref) return;
+/* Remove all consumer group references to a specific stream message.
+ * Returns 1 if any references were removed, otherwise 0. */
+int streamCleanupEntryCGroupRefs(stream *s, streamID *id) {
+    if (!s->cgroups_ref) return 0;
     list *cglist;
     listIter li;
     listNode *ln;
@@ -3271,7 +3272,7 @@ void streamCleanupEntryCGroupRefs(stream *s, streamID *id) {
 
     /* If message is not in any consumer group, nothing to do */
     if (!raxFind(s->cgroups_ref, buf, sizeof(streamID), (void **)&cglist))
-        return;
+        return 0;
 
     listRewind(cglist, &li);
     while ((ln = listNext(&li))) {
@@ -3293,6 +3294,7 @@ void streamCleanupEntryCGroupRefs(stream *s, streamID *id) {
 
     raxRemove(s->cgroups_ref, buf, sizeof(streamID), NULL);
     listRelease(cglist);
+    return 1;
 }
 
 /* Check if a stream entry is still referenced by any consumer group.
@@ -5108,10 +5110,11 @@ void xdelexCommand(client *c) {
     stream *s = kv->ptr;
     size_t old_alloc = server.memory_tracking_enabled ? kvobjAllocSize(kv) : 0;
     int first_entry = 0;
-    int deleted = 0;
+    int deleted = 0, dirty = server.dirty;
     addReplyArrayLen(c, args.numids);
     for (int j = 0; j < args.numids; j++) {
         int res = XDELEX_NO_ID;
+        int modified = 0;
         streamID *id = &ids[j];
         unsigned char buf[sizeof(streamID)];
         streamEncodeID(buf,id);
@@ -5122,7 +5125,7 @@ void xdelexCommand(client *c) {
             if (streamEntryIsReferenced(s, id))
                 can_delete = 0;
         } else if (args.delete_strategy == DELETE_STRATEGY_DELREF) {
-            streamCleanupEntryCGroupRefs(s, id);
+            modified = streamCleanupEntryCGroupRefs(s, id);
         }
 
         if (can_delete) { /* can_delete being true doesn't guarantee the ID exists */
@@ -5137,6 +5140,7 @@ void xdelexCommand(client *c) {
                     s->max_deleted_entry_id = *id;
                 }
                 deleted++;
+                modified = 1;
                 res = XDELEX_DELETED;
             } else {
                 /* This id doesn't exist. */
@@ -5145,13 +5149,15 @@ void xdelexCommand(client *c) {
             res = XDELEX_STILL_REFERENCED;
         }
 
+        if (modified) server.dirty++;
         addReplyLongLong(c, res);
     }
 
+    if (server.memory_tracking_enabled)
+        updateSlotAllocSize(c->db,getKeySlot(c->argv[1]->ptr),kv,old_alloc,kvobjAllocSize(kv));
+
     /* Update the stream's first ID. */
     if (deleted) {
-        if (server.memory_tracking_enabled)
-            updateSlotAllocSize(c->db,getKeySlot(c->argv[1]->ptr),kv,old_alloc,kvobjAllocSize(kv));
         if (s->length == 0) {
             s->first_id.ms = 0;
             s->first_id.seq = 0;
@@ -5162,7 +5168,9 @@ void xdelexCommand(client *c) {
         /* Propagate the write. */
         keyModified(c,c->db,c->argv[1],kv,1);
         notifyKeyspaceEvent(NOTIFY_STREAM,"xdel",c->argv[1],c->db->id);
-        server.dirty += deleted;
+    } else if (server.dirty > dirty) {
+        /* Only PEL references were removed, update LRM without signaling. */
+        keyModified(c,c->db,c->argv[1],kv,0);
     }
 
 cleanup:
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-00f3f3c65f` → 本草稿移入 `cases/defect/auto-redis-00f3f3c65f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
