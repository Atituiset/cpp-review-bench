# auto-curl-1b5bd69009

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #6193 (https://github.com/curl/curl/pull/6193)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 805；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -802,7 +802,11 @@ struct proxy_info {
 /* struct for HTTP CONNECT state data */
 struct http_connect_state {
   struct dynbuf rcvbuf;
-  int keepon;
+  enum keeponval {
+    KEEPON_DONE,
+    KEEPON_CONNECT,
+    KEEPON_IGNORE
+  } keepon;
   curl_off_t cl; /* size of content to read and ignore */
   enum {
     TUNNEL_INIT,    /* init/default/no tunnel state */
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-1b5bd69009` → 本草稿移入 `cases/defect/auto-curl-1b5bd69009/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
