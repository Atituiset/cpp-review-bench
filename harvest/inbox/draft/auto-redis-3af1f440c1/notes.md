# auto-redis-3af1f440c1

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #14907 (https://github.com/redis/redis/pull/14907)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 4013；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -4010,7 +4010,7 @@ static void rdbChannelReplDataBufClear(void) {
 static int replDataBufReadIntoLastBlock(connection *conn, replDataBuf *buf,
                                     void (*error_handler)(connection *conn))
 {
-    atomicIncr(server.stat_io_reads_processed[IOTHREAD_MAIN_THREAD_ID], 1);
+    atomicIncr(IOThreads[IOTHREAD_MAIN_THREAD_ID].io_reads_processed, 1);
 
     replDataBufBlock *block = listNodeValue(listLast(buf->blocks));
     serverAssert(block && block->size > block->used);
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-3af1f440c1` → 本草稿移入 `cases/defect/auto-redis-3af1f440c1/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
