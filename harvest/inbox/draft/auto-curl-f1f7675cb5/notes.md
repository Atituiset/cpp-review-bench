# auto-curl-f1f7675cb5

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #8049 (https://github.com/curl/curl/pull/8049)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -48,6 +48,18 @@ typedef enum {
   CURLUE_NO_PORT,             /* 15 */
   CURLUE_NO_QUERY,            /* 16 */
   CURLUE_NO_FRAGMENT,         /* 17 */
+  CURLUE_NO_ZONEID,           /* 18 */
+  CURLUE_BAD_FILE_URL,        /* 19 */
+  CURLUE_BAD_FRAGMENT,        /* 20 */
+  CURLUE_BAD_HOSTNAME,        /* 21 */
+  CURLUE_BAD_IPV6,            /* 22 */
+  CURLUE_BAD_LOGIN,           /* 23 */
+  CURLUE_BAD_PASSWORD,        /* 24 */
+  CURLUE_BAD_PATH,            /* 25 */
+  CURLUE_BAD_QUERY,           /* 26 */
+  CURLUE_BAD_SCHEME,          /* 27 */
+  CURLUE_BAD_SLASHES,         /* 28 */
+  CURLUE_BAD_USER,            /* 29 */
   CURLUE_LAST
 } CURLUcode;
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-f1f7675cb5` → 本草稿移入 `cases/defect/auto-curl-f1f7675cb5/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
