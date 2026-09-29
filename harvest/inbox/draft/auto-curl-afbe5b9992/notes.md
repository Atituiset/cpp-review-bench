# auto-curl-afbe5b9992

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #11799 (https://github.com/curl/curl/pull/11799)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 209；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -206,7 +206,7 @@ int main(int argc, char *argv[])
   CURL *curl;
   CURLcode res = CURLE_OK;
   const char *url;
-  curl_off_t l1, l2;
+  long l1, l2;
   size_t plen_min, plen_max;
 
 
@@ -217,12 +217,12 @@ int main(int argc, char *argv[])
   url = argv[1];
   l1 = strtol(argv[2], NULL, 10);
   if(l1 < 0) {
-    fprintf(stderr, "minlen must be >= 0, got %ld\n", (long)l1);
+    fprintf(stderr, "minlen must be >= 0, got %ld\n", l1);
     return 2;
   }
   l2 = strtol(argv[3], NULL, 10);
   if(l2 < 0) {
-    fprintf(stderr, "maxlen must be >= 0, got %ld\n", (long)l2);
+    fprintf(stderr, "maxlen must be >= 0, got %ld\n", l2);
     return 2;
   }
   plen_min = l1;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-afbe5b9992` → 本草稿移入 `cases/defect/auto-curl-afbe5b9992/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
