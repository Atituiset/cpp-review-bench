# auto-redis-1ced8f0db8

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15539 (https://github.com/redis/redis/pull/15539)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1868；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1865,7 +1865,14 @@ int loadSingleAppendOnlyFile(char *filename) {
 
         if (fseek(fp,0,SEEK_SET) == -1) goto readerr;
         rioInitWithFile(&rdb,fp);
-        if (rdbLoadRio(&rdb,RDBFLAGS_AOF_PREAMBLE,NULL) != C_OK) {
+
+        /* Active defrag doesn't run during regular RDB loading. Pause it
+         * while loading an RDB preamble from AOF as well. */
+        server.active_defrag_paused++;
+        int rdb_ret = rdbLoadRio(&rdb,RDBFLAGS_AOF_PREAMBLE,NULL);
+        server.active_defrag_paused--;
+
+        if (rdb_ret != C_OK) {
             if (old_style)
                 serverLog(LL_WARNING, "Error reading the RDB preamble of the AOF file %s, AOF loading aborted", filename);
             else
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-1ced8f0db8` → 本草稿移入 `cases/defect/auto-redis-1ced8f0db8/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
