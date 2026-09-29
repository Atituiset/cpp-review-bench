# auto-curl-bfa322b9d9

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #8049 (https://github.com/curl/curl/pull/8049)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 471；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -468,10 +468,10 @@ curl_url_strerror(CURLUcode error)
     return "An invalid 'part' argument was passed as argument";
 
   case CURLUE_MALFORMED_INPUT:
-    return "A malformed input was passed to a URL API function";
+    return "Malformed input to a URL function";
 
   case CURLUE_BAD_PORT_NUMBER:
-    return "The port number was not a decimal number between 0 and 65535";
+    return "Port number was not a decimal number between 0 and 65535";
 
   case CURLUE_UNSUPPORTED_SCHEME:
     return "This libcurl build doesn't support the given URL scheme";
@@ -489,28 +489,64 @@ curl_url_strerror(CURLUcode error)
     return "An unknown part ID was passed to a URL API function";
 
   case CURLUE_NO_SCHEME:
-    return "There is no scheme part in the URL";
+    return "No scheme part in the URL";
 
   case CURLUE_NO_USER:
-    return "There is no user part in the URL";
+    return "No user part in the URL";
 
   case CURLUE_NO_PASSWORD:
-    return "There is no password part in the URL";
+    return "No password part in the URL";
 
   case CURLUE_NO_OPTIONS:
-    return "There is no options part in the URL";
+    return "No options part in the URL";
 
   case CURLUE_NO_HOST:
-    return "There is no host part in the URL";
+    return "No host part in the URL";
 
   case CURLUE_NO_PORT:
-    return "There is no port part in the URL";
+    return "No port part in the URL";
 
   case CURLUE_NO_QUERY:
-    return "There is no query part in the URL";
+    return "No query part in the URL";
 
   case CURLUE_NO_FRAGMENT:
-    return "There is no fragment part in the URL";
+    return "No fragment part in the URL";
+
+  case CURLUE_NO_ZONEID:
+    return "No zoneid part in the URL";
+
+  case CURLUE_BAD_LOGIN:
+    return "Bad login part";
+
+  case CURLUE_BAD_IPV6:
+    return "Bad IPv6 address";
+
+  case CURLUE_BAD_HOSTNAME:
+    return "Bad hostname";
+
+  case CURLUE_BAD_FILE_URL:
+    return "Bad file:// URL";
+
+  case CURLUE_BAD_SLASHES:
+    return "Unsupported number of slashes";
+
+  case CURLUE_BAD_SCHEME:
+    return "Bad scheme";
+
+  case CURLUE_BAD_PATH:
+    return "Bad path";
+
+  case CURLUE_BAD_FRAGMENT:
+    return "Bad fragment";
+
+  case CURLUE_BAD_QUERY:
+    return "Bad query";
+
+  case CURLUE_BAD_PASSWORD:
+    return "Bad password";
+
+  case CURLUE_BAD_USER:
+    return "Bad user";
 
   case CURLUE_LAST:
     break;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-bfa322b9d9` → 本草稿移入 `cases/defect/auto-curl-bfa322b9d9/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
