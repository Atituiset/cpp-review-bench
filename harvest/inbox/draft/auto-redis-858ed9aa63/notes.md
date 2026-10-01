# auto-redis-858ed9aa63

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15628 (https://github.com/redis/redis/pull/15628)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 1（原始 PR diff 行 1703；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1697,43 +1697,34 @@ static doneStatus defragLuaScripts(void *ctx, monotime endtime) {
     return DEFRAG_DONE;
 }
 
-static doneStatus defragStageHashTemplates(void *ctx, monotime endtime) {
-    unsigned long *cursor = ctx;
-    hashTemplateRegistry *reg = server.htemplates;
-    if (reg == NULL || reg->by_id == NULL) return DEFRAG_DONE;
-
-    unsigned long iterations = 0;
-    while (*cursor < reg->by_id_next) {
-        hashTemplate *tmpl = hashTemplateGetById(*cursor);
-        (*cursor)++;
-        if (tmpl == NULL) continue; /* freed or never-used slot */
-
-        hashTemplateDefrag(tmpl);
-
-        if (++iterations > 8) {
-            iterations = 0;
-            if (getMonotonicUs() >= endtime) return DEFRAG_NOT_DONE;
-        }
-    }
-    return DEFRAG_DONE;
-}
-
 static void defragTmplRegistryCb(void *privdata, const dictEntry *de, dictEntryLink plink) {
-    UNUSED(privdata); 
-    UNUSED(de);
+    monotime endtime = *(monotime *)privdata;
     UNUSED(plink);
+    hashTemplateDefrag(dictGetKey(de), (dictEntry *)de, endtime);
 }
 
-/* Defrag a registry lookup dict. Keys/values (template/blob pointers) are
- * relocated by defragStageHashTemplates. */
-static doneStatus defragRegistryDict(dict **dref, unsigned long *cursor, monotime endtime) {
+/* Defrag a registry lookup dict, invoking 'scan_fn' per entry, the way
+ * defragStageKvstoreHelper() does. */
+static doneStatus defragRegistryDict(dict **dref, unsigned long *cursor,
+                                     dictScanFunction *scan_fn, monotime endtime)
+{
     dictDefragFunctions fns = { .defragAlloc = activeDefragAlloc };
-    unsigned long iterations = 0;
+    unsigned int iterations = 0;
+    unsigned long long prev_defragged = server.stat_active_defrag_hits;
+    unsigned long long prev_scanned = server.stat_active_defrag_scanned;
     do {
-        *cursor = dictScanDefrag(*dref, *cursor, defragTmplRegistryCb, &fns, NULL);
-        if (++iterations > 64) {
+        *cursor = dictScanDefrag(*dref, *cursor, scan_fn, &fns, &endtime);
+
+        if (++iterations > 16 ||
+            server.stat_active_defrag_hits - prev_defragged > 512 ||
+            server.stat_active_defrag_scanned - prev_scanned > 64)
+        {
             iterations = 0;
-            if (getMonotonicUs() >= endtime) return DEFRAG_NOT_DONE;
+            prev_defragged = server.stat_active_defrag_hits;
+            prev_scanned = server.stat_active_defrag_scanned;
+
+            if (*cursor != 0 && getMonotonicUs() >= endtime)
+                return DEFRAG_NOT_DONE;
         }
     } while (*cursor != 0);
     dict *newd = dictDefragTables(*dref);
@@ -1743,12 +1734,12 @@ static doneStatus defragRegistryDict(dict **dref, unsigned long *cursor, monotim
 
 static doneStatus defragStageHashTemplatesByFields(void *ctx, monotime endtime) {
     if (server.htemplates == NULL) return DEFRAG_DONE;
-    return defragRegistryDict(&server.htemplates->by_fields, ctx, endtime);
+    return defragRegistryDict(&server.htemplates->by_fields, ctx, defragTmplRegistryCb, endtime);
 }
 
 static doneStatus defragStageHashTemplatesByFieldsLp(void *ctx, monotime endtime) {
     if (server.htemplates == NULL) return DEFRAG_DONE;
-    return defragRegistryDict(&server.htemplates->by_fields_lp, ctx, endtime);
+    return defragRegistryDict(&server.htemplates->by_fields_lp, ctx, scanCallbackCountScanned, endtime);
 }
 
 static doneStatus defragStageHashTemplatesById(void *ctx, monotime endtime) {
@@ -2097,7 +2088,6 @@ static void beginDefragCycle(void) {
     addDefragStage(defragLuaScripts, NULL, NULL);
 
     /* Add stage for the hash template registry. */
-    addDefragStage(defragStageHashTemplates, zfree, zcalloc(sizeof(unsigned long)));
     addDefragStage(defragStageHashTemplatesByFields, zfree, zcalloc(sizeof(unsigned long)));
     addDefragStage(defragStageHashTemplatesByFieldsLp, zfree, zcalloc(sizeof(unsigned long)));
     addDefragStage(defragStageHashTemplatesById, zfree, zcalloc(sizeof(unsigne
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-858ed9aa63` → 本草稿移入 `cases/defect/auto-redis-858ed9aa63/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
