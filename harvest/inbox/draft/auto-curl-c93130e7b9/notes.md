# auto-curl-c93130e7b9

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #15289 (https://github.com/curl/curl/pull/15289)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 58；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -54,11 +54,7 @@
 extern "C" {
 #endif
 
-#if defined(BUILDING_LIBCURL) || defined(CURL_STRICTER)
-typedef struct Curl_multi CURLM;
-#else
 typedef void CURLM;
-#endif
 
 typedef enum {
   CURLM_CALL_MULTI_PERFORM = -1, /* please call curl_multi_perform() or
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-c93130e7b9` → 本草稿移入 `cases/defect/auto-curl-c93130e7b9/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
