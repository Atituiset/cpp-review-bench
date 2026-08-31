# auto-curl-1a3c0ef2dc

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #15289 (https://github.com/curl/curl/pull/15289)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 35；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -32,30 +32,28 @@
 
 #define HMAC_MD5_LENGTH 16
 
-typedef CURLcode (* HMAC_hinit_func)(void *context);
-typedef void    (* HMAC_hupdate_func)(void *context,
-                                      const unsigned char *data,
-                                      unsigned int len);
-typedef void    (* HMAC_hfinal_func)(unsigned char *result, void *context);
-
+typedef CURLcode (*HMAC_hinit)(void *context);
+typedef void    (*HMAC_hupdate)(void *context,
+                                const unsigned char *data,
+                                unsigned int len);
+typedef void    (*HMAC_hfinal)(unsigned char *result, void *context);
 
 /* Per-hash function HMAC parameters. */
 struct HMAC_params {
-  HMAC_hinit_func
-  hmac_hinit;     /* Initialize context procedure. */
-  HMAC_hupdate_func     hmac_hupdate;   /* Update context with data. */
-  HMAC_hfinal_func      hmac_hfinal;    /* Get final result procedure. */
-  unsigned int          hmac_ctxtsize;  /* Context structure size. */
-  unsigned int          hmac_maxkeylen; /* Maximum key length (bytes). */
-  unsigned int          hmac_resultlen; /* Result length (bytes). */
+  HMAC_hinit       hinit;     /* Initialize context procedure. */
+  HMAC_hupdate     hupdate;   /* Update context with data. */
+  HMAC_hfinal      hfinal;    /* Get final result procedure. */
+  unsigned int     ctxtsize;  /* Context structure size. */
+  unsigned int     maxkeylen; /* Maximum key length (bytes). */
+  unsigned int     resultlen; /* Result length (bytes). */
 };
 
 
 /* HMAC computation context. */
 struct HMAC_context {
-  const struct HMAC_params *hmac_hash; /* Hash function definition. */
-  void *hmac_hashctxt1;         /* Hash function context 1. */
-  void *hmac_hashctxt2;         /* Hash function context 2. */
+  const struct HMAC_params *hash; /* Hash function definition. */
+  void *hashctxt1;         /* Hash function context 1. */
+  void *hashctxt2;         /* Hash function context 2. */
 };
 
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-1a3c0ef2dc` → 本草稿移入 `cases/defect/auto-curl-1a3c0ef2dc/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
