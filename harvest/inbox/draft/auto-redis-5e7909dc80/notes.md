# auto-redis-5e7909dc80

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15190 (https://github.com/redis/redis/pull/15190)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 109；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -56,8 +56,13 @@ void vecRelease(vec *v) {
     v->free = NULL;
 }
 
-/* Reset the logical length to zero while preserving allocated storage. */
+/* Reset the logical length to zero while preserving allocated storage.
+ * If a free method is set, it is applied to every element before reset. */
 void vecClear(vec *v) {
+    if (v->free) {
+        for (size_t i = 0; i < v->size; i++)
+            v->free(v->data[i]);
+    }
     v->size = 0;
 }
 
@@ -106,8 +111,14 @@ void vecPush(vec *v, void *value) {
 
 static int vecTestFreeCalls = 0;
 static void vecTestFree(void *ptr) {
-    UNUSED(ptr);
     vecTestFreeCalls++;
+    zfree(ptr);
+}
+
+static int *vecTestNewInt(int v) {
+    int *p = zmalloc(sizeof(int));
+    *p = v;
+    return p;
 }
 
 int vectorTest(int argc, char **argv, int flags)
@@ -175,14 +186,35 @@ int vectorTest(int argc, char **argv, int flags)
     void *vstack2[2];
     vecInit(&v, vstack2, 2);
     vecSetFreeMethod(&v, vecTestFree);
-    vecPush(&v, &one);
-    vecPush(&v, &two);
-    vecPush(&v, &three); /* triggers spill to heap */
+    vecPush(&v, vecTestNewInt(1));
+    vecPush(&v, vecTestNewInt(2));
+    vecPush(&v, vecTestNewInt(3)); /* triggers spill to heap */
     vecTestFreeCalls = 0;
     vecRelease(&v);
     test_cond("vecRelease() invokes free method on each element",
               vecTestFreeCalls == 3);
 
+    /* vecClear: free method is invoked on each element, storage preserved. */
+    vecInit(&v, NULL, 4);
+    vecSetFreeMethod(&v, vecTestFree);
+    vecPush(&v, vecTestNewInt(1));
+    vecPush(&v, vecTestNewInt(2));
+    vecPush(&v, vecTestNewInt(3));
+    heap_data = vecData(&v);
+    vecTestFreeCalls = 0;
+    vecClear(&v);
+    test_cond("vecClear() invokes free method on each element preserving storage",
+              vecTestFreeCalls == 3 && vecSize(&v) == 0 &&
+              vecData(&v) == heap_data && v.cap == 4);
+    /* Push again after clear to verify the vector is still usable. */
+    vecPush(&v, vecTestNewInt(4));
+    test_cond("vecPush() works after vecClear() with free method",
+              vecSize(&v) == 1 && vecData(&v) == heap_data);
+    vecTestFreeCalls = 0;
+    vecRelease(&v);
+    test_cond("vecRelease() after vecClear()+push frees remaining element",
+              vecTestFreeCalls == 1);
+
     vecInit(&v, NULL, 4);
     vecSetFreeMethod(&v, vecTestFree);
     vecTestFreeCalls = 0;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-5e7909dc80` → 本草稿移入 `cases/defect/auto-redis-5e7909dc80/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
