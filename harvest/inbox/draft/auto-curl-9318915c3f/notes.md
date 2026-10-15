# auto-curl-9318915c3f

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #8035 (https://github.com/curl/curl/pull/8035)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 8（原始 PR diff 行 1725；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -63,7 +63,7 @@
 #include "tool_filetime.h"
 #include "tool_getparam.h"
 #include "tool_helpers.h"
-#include "tool_homedir.h"
+#include "tool_findfile.h"
 #include "tool_libinfo.h"
 #include "tool_main.h"
 #include "tool_msgs.h"
@@ -1716,23 +1716,19 @@ static CURLcode single_transfer(struct GlobalConfig *global,
 
         if((use_proto & (CURLPROTO_SCP|CURLPROTO_SFTP)) &&
            !config->insecure_ok) {
-          char *home = homedir(NULL);
-          if(home) {
-            char *file = aprintf("%s/.ssh/known_hosts", home);
-            if(file) {
-              /* new in curl 7.19.6 */
-              result = res_setopt_str(curl, CURLOPT_SSH_KNOWNHOSTS, file);
-              curl_free(file);
-              if(result == CURLE_UNKNOWN_OPTION)
-                /* libssh2 version older than 1.1.1 */
-                result = CURLE_OK;
-            }
-            Curl_safefree(home);
+          char *known = findfile(".ssh/known_hosts", FALSE);
+          if(known) {
+            /* new in curl 7.19.6 */
+            result = res_setopt_str(curl, CURLOPT_SSH_KNOWNHOSTS, known);
+            curl_free(known);
+            if(result == CURLE_UNKNOWN_OPTION)
+              /* libssh2 version older than 1.1.1 */
+              result = CURLE_OK;
             if(result)
               break;
           }
           else
-            warnf(global, "No home dir, couldn't find known_hosts file!");
+            warnf(global, "Couldn't find a known_hosts file!");
         }
 
         if(config->no_body || config->remote_time) {
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-9318915c3f` → 本草稿移入 `cases/defect/auto-curl-9318915c3f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
