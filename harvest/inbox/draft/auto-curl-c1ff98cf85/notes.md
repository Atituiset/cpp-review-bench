# auto-curl-c1ff98cf85

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #0dc22b690dd8dba4048d494f09a50122dd7c0dd4 (https://github.com/curl/curl/commit/0dc22b690dd8dba4048d494f09a50122dd7c0dd4)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 383；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -380,9 +380,9 @@ static void cf_ssl_scache_session_ldestroy(void *udata, void *obj)
 {
   struct Curl_ssl_session *s = obj;
   (void)udata;
-  curlx_free(CURL_UNCONST(s->sdata));
-  curlx_free(CURL_UNCONST(s->quic_tp));
-  curlx_free((void *)s->alpn);
+  curlx_free(s->sdata);
+  curlx_free(s->quic_tp);
+  curlx_free(s->alpn);
   curlx_free(s);
 }
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-c1ff98cf85` → 本草稿移入 `cases/defect/auto-curl-c1ff98cf85/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
