# auto-curl-9318915c3f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | curl/curl |
| 源 PR | [#8035](https://github.com/curl/curl/pull/8035) |
| 许可证 | MIT |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-01 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 11 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #8035 (https://github.com/curl/curl/pull/8035)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1725；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 8035 curl: modified search for files in "homedir" :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

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

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`aprintf`
- 外部函数：`curl_free`
- 外部函数：`homedir`
- 外部函数：`res_setopt_str`
- 外部函数：`warnf`
- 大写宏：`CURLE_OK`
- 大写宏：`CURLE_UNKNOWN_OPTION`
- 大写宏：`CURLOPT_SSH_KNOWNHOSTS`
- 大写宏：`CURLPROTO_SCP`
- 大写宏：`CURLPROTO_SFTP`
- 大写宏：`NULL`
- 外部类型：`No`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。
- **依赖重（dep_count≥10）**：可考虑只做 PR/diff 形态评审，不做独立 case。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-curl-9318915c3f` → 本草稿移入 `cases/defect/auto-curl-9318915c3f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
