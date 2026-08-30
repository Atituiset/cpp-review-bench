# auto-nginx-6cd7a33d87

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #514 (https://github.com/nginx/nginx/pull/514)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -52,7 +52,6 @@
 #include <malloc.h>             /* memalign() */
 #include <limits.h>             /* IOV_MAX */
 #include <sys/ioctl.h>
-#include <crypt.h>
 #include <sys/utsname.h>        /* uname() */
 
 #include <dlfcn.h>
@@ -61,6 +60,11 @@
 #include <ngx_auto_config.h>
 
 
+#if (NGX_HAVE_CRYPT_H)
+#include <crypt.h>
+#endif
+
+
 #if (NGX_HAVE_POSIX_SEM)
 #include <semaphore.h>
 #endif
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-6cd7a33d87` → 本草稿移入 `cases/defect/auto-nginx-6cd7a33d87/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
