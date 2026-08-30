# auto-redis-48760a3452

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #14934 (https://github.com/redis/redis/pull/14934)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 352；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -349,7 +349,7 @@ size_t freeMemoryGetNotCountedMemory(void) {
     /* The migrate client is like a replica, we also push DELs into it when
      * evicting keys belonging to the migrating slot, so we don't count its
      * output buffer to avoid eviction loop. */
-    overhead += asmGetMigrateOutputBufferSize();
+    overhead += asmGetMigrateOutputMemoryUsage();
 
     if (server.aof_state != AOF_OFF) {
         overhead += sdsAllocSize(server.aof_buf);
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-48760a3452` → 本草稿移入 `cases/defect/auto-redis-48760a3452/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
