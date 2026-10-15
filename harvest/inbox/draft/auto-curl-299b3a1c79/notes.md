# auto-curl-299b3a1c79

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #7808 (https://github.com/curl/curl/pull/7808)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -39,6 +39,20 @@
 #endif
 #endif /* USE_MBEDTLS */
 
+#if defined(USE_OPENSSL) && !defined(USE_AMISSL)
+  #include <openssl/opensslconf.h>
+  #if !defined(OPENSSL_NO_MD5) && !defined(OPENSSL_NO_DEPRECATED_3_0)
+    #define USE_OPENSSL_MD5
+  #endif
+#endif
+
+#ifdef USE_WOLFSSL
+  #include <wolfssl/options.h>
+  #ifndef NO_MD5
+    #define USE_WOLFSSL_MD5
+  #endif
+#endif
+
 #if defined(USE_GNUTLS)
 
 #include <nettle/md5.h>
@@ -65,19 +79,13 @@ static void MD5_Final(unsigned char *digest, MD5_CTX *ctx)
   md5_digest(ctx, 16, digest);
 }
 
-#elif (defined(USE_OPENSSL) && !defined(USE_AMISSL)) || defined(USE_WOLFSSL)
+#elif defined(USE_OPENSSL_MD5) || defined(USE_WOLFSSL_MD5)
 
-#ifdef USE_WOLFSSL
-#include <wolfssl/options.h>
-#endif
-
-#if defined(USE_OPENSSL) || (defined(USE_WOLFSSL) && !defined(NO_MD5))
 /* When OpenSSL or wolfSSL is available, we use their MD5 functions. */
 #include <openssl/md5.h>
 #include "curl_memory.h"
 /* The last #include file should be: */
 #include "memdebug.h"
-#endif
 
 #elif defined(USE_MBEDTLS)
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-299b3a1c79` → 本草稿移入 `cases/defect/auto-curl-299b3a1c79/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
