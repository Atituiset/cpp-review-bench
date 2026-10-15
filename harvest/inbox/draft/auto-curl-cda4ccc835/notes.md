# auto-curl-cda4ccc835

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #2443 (https://github.com/curl/curl/pull/2443)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 512；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -507,30 +507,30 @@ main ()
 #ifdef HAVE_GLIBC_STRERROR_R
 #include <string.h>
 #include <errno.h>
+
+void check(char c) {}
+
 int
 main () {
-  char buffer[1024]; /* big enough to play with */
-  char *string =
-    strerror_r(EACCES, buffer, sizeof(buffer));
-    /* this should've returned a string */
-    if(!string || !string[0])
-      return 99;
-    return 0;
+  char buffer[1024];
+  // This will not compile if strerror_r does not return a char*
+  check(strerror_r(EACCES, buffer, sizeof(buffer))[0]);
+  return 0;
 }
 #endif
 #ifdef HAVE_POSIX_STRERROR_R
 #include <string.h>
 #include <errno.h>
+
+// float, because a pointer can't be implicitly cast to float
+void check(float f) {}
+
 int
 main () {
-  char buffer[1024]; /* big enough to play with */
-  int error =
-    strerror_r(EACCES, buffer, sizeof(buffer));
-    /* This should've returned zero, and written an error string in the
-       buffer.*/
-    if(!buffer[0] || error)
-      return 99;
-    return 0;
+  char buffer[1024];
+  // This will not compile if strerror_r does not return an int
+  check(strerror_r(EACCES, buffer, sizeof(buffer)));
+  return 0;
 }
 #endif
 #ifdef HAVE_FSETXATTR_6
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-cda4ccc835` → 本草稿移入 `cases/defect/auto-curl-cda4ccc835/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
