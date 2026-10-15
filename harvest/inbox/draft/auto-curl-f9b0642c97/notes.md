# auto-curl-f9b0642c97

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #15155 (https://github.com/curl/curl/pull/15155)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 929；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -926,10 +926,10 @@ CURLcode Curl_cpool_add_pollfds(struct cpool *cpool,
   return result;
 }
 
-CURLcode Curl_cpool_add_waitfds(struct cpool *cpool,
-                                struct curl_waitfds *cwfds)
+unsigned int Curl_cpool_add_waitfds(struct cpool *cpool,
+                                    struct curl_waitfds *cwfds)
 {
-  CURLcode result = CURLE_OK;
+  unsigned int need = 0;
 
   CPOOL_LOCK(cpool);
   if(Curl_llist_head(&cpool->shutdowns)) {
@@ -945,14 +945,11 @@ CURLcode Curl_cpool_add_waitfds(struct cpool *cpool,
       Curl_conn_adjust_pollset(cpool->idata, &ps);
       Curl_detach_connection(cpool->idata);
 
-      result = Curl_waitfds_add_ps(cwfds, &ps);
-      if(result)
-        goto out;
+      need += Curl_waitfds_add_ps(cwfds, &ps);
     }
   }
-out:
   CPOOL_UNLOCK(cpool);
-  return result;
+  return need;
 }
 
 static void cpool_perform(struct cpool *cpool)
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-f9b0642c97` → 本草稿移入 `cases/defect/auto-curl-f9b0642c97/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
