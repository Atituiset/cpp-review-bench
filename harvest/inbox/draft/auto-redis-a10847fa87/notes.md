# auto-redis-a10847fa87

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #14608 (https://github.com/redis/redis/pull/14608)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -195,6 +195,88 @@ void freeObjAsync(robj *key, robj *obj, int dbid) {
     }
 }
 
+/* Duplicate client reply objects that reference database objects to avoid race
+ * conditions with bio threads during async flushdb.
+ *
+ * Since incrRefCount/decrRefCount are not thread-safe, and bio thread may
+ * free database objects while main thread/IO threads send client replies, we need to
+ * create independent copies of the string objects to avoid concurrent access. */
+static void protectClientReplyObjects(void) {
+    /* If there are no clients with pending ref replies, exit ASAP. */
+    if (!listLength(server.clients_with_pending_ref_reply))
+        return;
+
+    /* Pause all IO threads to safely duplicate string objects. */
+    int allpaused = 0;
+    if (server.io_threads_num > 1) {
+        serverAssert(pthread_equal(server.main_thread_id, pthread_self()));
+        allpaused = 1;
+        pauseAllIOThreads();
+    }
+
+    listNode *ln;
+    listIter li;
+    listRewind(server.clients_with_pending_ref_reply, &li);
+    while ((ln = listNext(&li)) != NULL) {
+        client *c = listNodeValue(ln);
+
+        /* Process c->buf if it's encoded */
+        if (c->buf_encoded && c->bufpos > 0) {
+            char *ptr = c->buf;
+            while (ptr < c->buf + c->bufpos) {
+                payloadHeader *header = (payloadHeader *)ptr;
+                ptr += sizeof(payloadHeader);
+
+                if (header->payload_type == BULK_STR_REF) {
+                    bulkStrRef *str_ref = (bulkStrRef *)ptr;
+                    if (str_ref->obj != NULL) {
+                        /* Duplicate the string object */
+                        robj *new_obj = dupStringObject(str_ref->obj);
+                        decrRefCount(str_ref->obj);
+                        str_ref->obj = new_obj;
+                    }
+                }
+                ptr += header->payload_len;
+            }
+        }
+
+        /* Process reply list */
+        if (c->reply && listLength(c->reply)) {
+            listIter reply_li;
+            listNode *reply_ln;
+            listRewind(c->reply, &reply_li);
+            while ((reply_ln = listNext(&reply_li))) {
+                clientReplyBlock *block = listNodeValue(reply_ln);
+                if (block && block->buf_encoded) {
+                    char *ptr = block->buf;
+                    while (ptr < block->buf + block->used) {
+                        payloadHeader *header = (payloadHeader *)ptr;
+                        ptr += sizeof(payloadHeader);
+
+                        if (header->payload_type == BULK_STR_REF) {
+                            bulkStrRef *str_ref = (bulkStrRef *)ptr;
+                            if (str_ref->obj != NULL) {
+                                /* Duplicate the string object */
+                                robj *new_obj = dupStringObject(str_ref->obj);
+                                decrRefCount(str_ref->obj);
+                                str_ref->obj = new_obj;
+                            }
+                        }
+                        ptr += header->payload_len;
+                    }
+                }
+            }
+        }
+
+        /* Process references in IO deferred objects and remove client from
+         * pending ref list since all refs have been duplicated above. */
+        freeClientIODeferredObjects(c, 0);
+        tryUnlinkClientFromPendingRefReply(c, 1);
+    }
+
+    if (allpaused) resumeAllIOThreads();
+}
+
 /* Empty a Redis DB asynchronously. What the function does actually is to
  * create a new empty set of hash tables and scheduling the old ones for
  * lazy freeing. */
@@ -210,6 +292,7 @@ void emptyDbAsync(redisDb *db) {
     db->keys = kvstoreCreate(&kvstoreExType, &dbDictType, slot_count_bits, flags);
     db->expires = kvstoreCreate(&kvstoreBaseType, &dbExpiresDictType, slot_count_bits, flags);
     db->subexpires = estoreCreate(&subexpiresBucketsType, slot_count_bits);
+    protectClientReplyObjects(); /* Pro
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-a10847fa87` → 本草稿移入 `cases/defect/auto-redis-a10847fa87/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
