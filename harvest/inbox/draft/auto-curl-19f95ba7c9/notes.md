# auto-curl-19f95ba7c9

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #15155 (https://github.com/curl/curl/pull/15155)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1204；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1200,8 +1200,9 @@ CURLMcode curl_multi_waitfds(CURLM *m,
   CURLMcode result = CURLM_OK;
   struct Curl_llist_node *e;
   struct Curl_multi *multi = m;
+  unsigned int need = 0;
 
-  if(!ufds)
+  if(!ufds && (size || !fd_count))
     return CURLM_BAD_FUNCTION_ARGUMENT;
 
   if(!GOOD_MULTI_HANDLE(multi))
@@ -1214,20 +1215,17 @@ CURLMcode curl_multi_waitfds(CURLM *m,
   for(e = Curl_llist_head(&multi->process); e; e = Curl_node_next(e)) {
     struct Curl_easy *data = Curl_node_elem(e);
     multi_getsock(data, &data->last_poll);
-    if(Curl_waitfds_add_ps(&cwfds, &data->last_poll)) {
-      result = CURLM_OUT_OF_MEMORY;
-      goto out;
-    }
+    need += Curl_waitfds_add_ps(&cwfds, &data->last_poll);
   }
 
-  if(Curl_cpool_add_waitfds(&multi->cpool, &cwfds)) {
+  need += Curl_cpool_add_waitfds(&multi->cpool, &cwfds);
+
+  if(need != cwfds.n && ufds) {
     result = CURLM_OUT_OF_MEMORY;
-    goto out;
   }
 
-out:
   if(fd_count)
-    *fd_count = cwfds.n;
+    *fd_count = need;
   return result;
 }
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-19f95ba7c9` → 本草稿移入 `cases/defect/auto-curl-19f95ba7c9/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
