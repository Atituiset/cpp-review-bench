# auto-redis-6562318fed

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #14721 (https://github.com/redis/redis/pull/14721)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -476,6 +476,10 @@ static void redisSSLFree(void *privctx){
 
     if (!rsc) return;
     if (rsc->ssl) {
+        /* Should not be called if a previous fatal error has occurred on a connection;
+         * i.e., if SSL_get_error(3) has returned SSL_ERROR_SYSCALL or SSL_ERROR_SSL.
+         * (https://docs.openssl.org/3.3/man3/SSL_shutdown/#description). */
+        SSL_shutdown(rsc->ssl);
         SSL_free(rsc->ssl);
         rsc->ssl = NULL;
     }
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-6562318fed` → 本草稿移入 `cases/defect/auto-redis-6562318fed/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
