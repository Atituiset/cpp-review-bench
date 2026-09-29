# auto-curl-551627ec17

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #15289 (https://github.com/curl/curl/pull/15289)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 118；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -30,7 +30,7 @@
  */
 
 #ifdef CURL_NO_OLDIES
-#define CURL_STRICTER
+#define CURL_STRICTER /* not used since 8.11.0 */
 #endif
 
 /* Compile-time deprecation macros. */
@@ -114,13 +114,8 @@
 extern "C" {
 #endif
 
-#if defined(BUILDING_LIBCURL) || defined(CURL_STRICTER)
-typedef struct Curl_easy CURL;
-typedef struct Curl_share CURLSH;
-#else
 typedef void CURL;
 typedef void CURLSH;
-#endif
 
 /*
  * libcurl external API function linkage decorations.
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-551627ec17` → 本草稿移入 `cases/defect/auto-curl-551627ec17/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
