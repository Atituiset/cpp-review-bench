# auto-redis-2d6fb7d4b2

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15103 (https://github.com/redis/redis/pull/15103)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1270；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1254,48 +1254,48 @@ int raxRemove(rax *rax, unsigned char *s, size_t len, void **old) {
     return 1;
 }
 
-/* This is the core of raxFree(): performs a depth-first scan of the
- * tree and releases all the nodes found. */
-void raxRecursiveFree(rax *rax, raxNode *n, void (*free_callback)(void*)) {
-    debugnode("free traversing",n);
-    int numchildren = n->iscompr ? 1 : n->size;
-    raxNode **cp = raxNodeLastChildPtr(n);
-    while(numchildren--) {
-        raxNode *child;
-        memcpy(&child,cp,sizeof(child));
-        raxRecursiveFree(rax,child,free_callback);
-        cp--;
+/* This is the core of raxFree(): performs an iterative depth-first scan
+ * of the tree and frees all the nodes found. Uses an explicit heap stack
+ * to avoid stack overflow on deep trees. The caller passes exactly one
+ * callback variant and the non-NULL one is invoked. */
+static void raxFreeNodesWithCallback(rax *rax, raxNode *n,
+                                     void (*free_callback)(void *item),
+                                     void (*free_callback_withctx)(void *item, void *ctx),
+                                     void *ctx)
+{
+    raxStack stack;
+    raxStackInit(&stack);
+    raxStackPush(&stack, n);
+
+    while (stack.items > 0) {
+        raxNode *curr = raxStackPop(&stack);
+        debugnode("free traversing",curr);
+        int numchildren = curr->iscompr ? 1 : curr->size;
+        raxNode **cp = raxNodeFirstChildPtr(curr);
+        for (int i = 0; i < numchildren; i++) {
+            raxNode *child;
+            memcpy(&child, cp + i, sizeof(child));
+            raxStackPush(&stack, child);
+        }
+        debugnode("free depth-first",curr);
+        if (curr->iskey && !curr->isnull) {
+            void *data = raxGetData(curr);
+            if (free_callback_withctx)
+                free_callback_withctx(data, ctx);
+            else if (free_callback)
+                free_callback(data);
+        }
+        raxFreeNode(rax, curr);
+        rax->numnodes--;
     }
-    debugnode("free depth-first",n);
-    if (free_callback && n->iskey && !n->isnull)
-        free_callback(raxGetData(n));
-    raxFreeNode(rax,n);
-    rax->numnodes--;
-}
 
-/* Same as raxRecursiveFree() with context argument */
-void raxRecursiveFreeWithCtx(rax *rax, raxNode *n,
-                            void (*free_callback)(void *item, void *ctx), void *ctx) {
-    debugnode("free traversing",n);
-    int numchildren = n->iscompr ? 1 : n->size;
-    raxNode **cp = raxNodeLastChildPtr(n);
-    while(numchildren--) {
-        raxNode *child;
-        memcpy(&child,cp,sizeof(child));
-        raxRecursiveFreeWithCtx(rax,child,free_callback, ctx);
-        cp--;
-    }
-    debugnode("free depth-first",n);
-    if (free_callback && n->iskey && !n->isnull)
-        free_callback(raxGetData(n), ctx);
-    raxFreeNode(rax,n);
-    rax->numnodes--;
+    raxStackFree(&stack);
 }
 
 /* Free a whole radix tree, calling the specified callback in order to
  * free the auxiliary data. */
 void raxFreeWithCallback(rax *rax, void (*free_callback)(void*)) {
-    raxRecursiveFree(rax,rax->head,free_callback);
+    raxFreeNodesWithCallback(rax, rax->head, free_callback, NULL, NULL);
     assert(rax->numnodes == 0);
     size_t *alloc_size = rax->alloc_size;
     size_t usable;
@@ -1307,7 +1307,7 @@ void raxFreeWithCallback(rax *rax, void (*free_callback)(void*)) {
  * free the auxiliary data. */
 void raxFreeWithCbAndContext(rax *rax,
                              void (*free_callback)(void *item, void *ctx), void *ctx) {
-    raxRecursiveFreeWithCtx(rax,rax->head,free_callback,ctx);
+    raxFreeNodesWithCallback(rax, rax->head, NULL, free_callback, ctx);
     assert(rax->numnodes == 0);
     size_t *alloc_size = rax->alloc_size;
     size_t usable;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-2d6fb7d4b2` → 本草稿移入 `cases/defect/auto-redis-2d6fb7d4b2/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
