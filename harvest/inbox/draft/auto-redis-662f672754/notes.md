# auto-redis-662f672754

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15467 (https://github.com/redis/redis/pull/15467)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 123；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -120,7 +120,6 @@ void enqueuePendingClientsToMainThread(client *c, int unbind) {
         sendPendingClientsToMainThreadIfNeeded(t, 1);
         /* Disable read and write to avoid race when main thread processes. */
         c->io_flags &= ~(CLIENT_IO_READ_ENABLED | CLIENT_IO_WRITE_ENABLED);
-        connSetWriteHandler(c->conn, NULL);
         /* Remove the client from IO thread, add it to main thread's pending list. */
         listUnlinkNode(t->clients, c->io_thread_client_list_node);
         listLinkNodeTail(t->pending_clients_to_main_thread, c->io_thread_client_list_node);
@@ -773,6 +772,13 @@ int processClientsFromMainThread(IOThread *t) {
                 connSetWriteHandler(c->conn, sendReplyToClient);
             }
         }
+        /* If everything has been flushed, drop any stale write handler so the IO
+         * thread event loop doesn't keep firing writable (EPOLLOUT) on an idle
+         * client. */
+        if (!(c->io_flags & CLIENT_IO_CLOSE_ASAP) && !clientHasPendingReplies(c)) {
+            serverAssert(connHasEventLoop(c->conn));
+            connSetWriteHandler(c->conn, NULL);
+        }
     }
     /* All clients must are processed. */
     serverAssert(listLength(t->processing_clients) == 0);
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-662f672754` → 本草稿移入 `cases/defect/auto-redis-662f672754/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
