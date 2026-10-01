# auto-curl-0f1d42e63c

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #16601 (https://github.com/curl/curl/pull/16601)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 2148；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -2145,12 +2145,16 @@ static CURLcode setopt_cptr(struct Curl_easy *data, CURLoption option,
     result = setstropt_userpwd(ptr, &u, &p);
 
     /* URL decode the components */
-    if(!result && u)
+    if(!result && u) {
+      Curl_safefree(data->set.str[STRING_PROXYUSERNAME]);
       result = Curl_urldecode(u, 0, &data->set.str[STRING_PROXYUSERNAME], NULL,
                               REJECT_ZERO);
-    if(!result && p)
+    }
+    if(!result && p) {
+      Curl_safefree(data->set.str[STRING_PROXYPASSWORD]);
       result = Curl_urldecode(p, 0, &data->set.str[STRING_PROXYPASSWORD], NULL,
                               REJECT_ZERO);
+    }
     free(u);
     free(p);
   }
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-0f1d42e63c` → 本草稿移入 `cases/defect/auto-curl-0f1d42e63c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
