# auto-redis-96cd9b3726

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15004 (https://github.com/redis/redis/pull/15004)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 2251；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1522,6 +1522,7 @@ void delexCommand(client *c) {
         rewriteClientCommandVector(c, 2, shared.del, key);
         keyModified(c, c->db, key, NULL, 1);
         notifyKeyspaceEvent(NOTIFY_GENERIC, "del", key, c->db->id);
+        KSN_INVALIDATE_KVOBJ(o);
         server.dirty++;
     }
 
@@ -2248,10 +2249,9 @@ void renameGenericCommand(client *c, int nx) {
 
     keyModified(c,c->db,c->argv[1],NULL,1);
     keyModified(c,c->db,c->argv[2],o,1);
-    notifyKeyspaceEvent(NOTIFY_GENERIC,"rename_from",
-        c->argv[1],c->db->id);
-    notifyKeyspaceEvent(NOTIFY_GENERIC,"rename_to",
-        c->argv[2],c->db->id);
+    notifyKeyspaceEvent(NOTIFY_GENERIC, "rename_from", c->argv[1],c->db->id);
+    notifyKeyspaceEvent(NOTIFY_GENERIC, "rename_to", c->argv[2],c->db->id);
+    KSN_INVALIDATE_KVOBJ(o);
     if (overwritten) {
         notifyKeyspaceEvent(NOTIFY_OVERWRITTEN, "overwritten", c->argv[2], c->db->id);
         if (desttype != srctype)
@@ -2346,10 +2346,9 @@ void moveCommand(client *c) {
 
     keyModified(c,src,c->argv[1],NULL,1);
     keyModified(c,dst,c->argv[1],kv,1);
-    notifyKeyspaceEvent(NOTIFY_GENERIC,
-                "move_from",c->argv[1],src->id);
-    notifyKeyspaceEvent(NOTIFY_GENERIC,
-                "move_to",c->argv[1],dst->id);
+    notifyKeyspaceEvent(NOTIFY_GENERIC, "move_from", c->argv[1],src->id);
+    notifyKeyspaceEvent(NOTIFY_GENERIC, "move_to", c->argv[1],dst->id);
+    KSN_INVALIDATE_KVOBJ(kv);
 
     server.dirty++;
     addReply(c,shared.cone);
@@ -2471,6 +2470,7 @@ void copyCommand(client *c) {
     /* OK! key copied. Signal modification */
     keyModified(c,dst,c->argv[2],kvCopy,1);
     notifyKeyspaceEvent(NOTIFY_GENERIC,"copy_to",c->argv[2],dst->id);
+    KSN_INVALIDATE_KVOBJ(kvCopy);
 
     /* `delete` implies the destination key was overwritten */
     if (delete) {
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-96cd9b3726` → 本草稿移入 `cases/defect/auto-redis-96cd9b3726/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
