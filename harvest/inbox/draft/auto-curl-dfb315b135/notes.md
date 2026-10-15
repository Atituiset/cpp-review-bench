# auto-curl-dfb315b135

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #970 (https://github.com/curl/curl/pull/970)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 268；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -265,8 +265,10 @@ CURLcode Curl_auth_create_spnego_message(struct Curl_easy *data,
   if(result)
     return result;
 
-  if(!*outptr || !*outlen)
+  if(!*outptr || !*outlen) {
+    free(*outptr);
     return CURLE_REMOTE_ACCESS_DENIED;
+  }
 
   return CURLE_OK;
 }
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-dfb315b135` → 本草稿移入 `cases/defect/auto-curl-dfb315b135/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
