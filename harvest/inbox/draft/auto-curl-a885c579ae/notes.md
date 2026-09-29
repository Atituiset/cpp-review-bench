# auto-curl-a885c579ae

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #17753 (https://github.com/curl/curl/pull/17753)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -3310,8 +3310,14 @@ static CURLcode http_header_r(struct Curl_easy *data,
 static CURLcode http_header_s(struct Curl_easy *data,
                               const char *hd, size_t hdlen)
 {
+#if !defined(CURL_DISABLE_COOKIES) || !defined(CURL_DISABLE_HSTS)
   struct connectdata *conn = data->conn;
   const char *v;
+#else
+  (void)data;
+  (void)hd;
+  (void)hdlen;
+#endif
 
 #if !defined(CURL_DISABLE_COOKIES)
   v = (data->cookies && data->state.cookie_engine) ?
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-a885c579ae` → 本草稿移入 `cases/defect/auto-curl-a885c579ae/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
