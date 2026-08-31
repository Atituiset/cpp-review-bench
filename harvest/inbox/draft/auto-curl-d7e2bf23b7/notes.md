# auto-curl-d7e2bf23b7

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #7808 (https://github.com/curl/curl/pull/7808)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 43；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -40,7 +40,7 @@
 
 #include <openssl/opensslv.h>
 
-#if (OPENSSL_VERSION_NUMBER >= 0x0090800fL)
+#if (OPENSSL_VERSION_NUMBER >= 0x0090700fL)
 #define USE_OPENSSL_SHA256
 #endif
 
@@ -70,7 +70,36 @@
 #if defined(USE_OPENSSL_SHA256)
 
 /* When OpenSSL is available we use the SHA256-function from OpenSSL */
-#include <openssl/sha.h>
+#include <openssl/evp.h>
+
+#include "curl_memory.h"
+
+/* The last #include file should be: */
+#include "memdebug.h"
+
+struct sha256_ctx {
+  EVP_MD_CTX *openssl_ctx;
+};
+typedef struct sha256_ctx my_sha256_ctx;
+
+static void my_sha256_init(my_sha256_ctx *ctx)
+{
+  ctx->openssl_ctx = EVP_MD_CTX_create();
+  EVP_DigestInit_ex(ctx->openssl_ctx, EVP_sha256(), NULL);
+}
+
+static void my_sha256_update(my_sha256_ctx *ctx,
+                             const unsigned char *data,
+                             unsigned int length)
+{
+  EVP_DigestUpdate(ctx->openssl_ctx, data, length);
+}
+
+static void my_sha256_final(unsigned char *digest, my_sha256_ctx *ctx)
+{
+  EVP_DigestFinal_ex(ctx->openssl_ctx, digest, NULL);
+  EVP_MD_CTX_destroy(ctx->openssl_ctx);
+}
 
 #elif defined(USE_GNUTLS)
 
@@ -81,21 +110,21 @@
 /* The last #include file should be: */
 #include "memdebug.h"
 
-typedef struct sha256_ctx SHA256_CTX;
+typedef struct sha256_ctx my_sha256_ctx;
 
-static void SHA256_Init(SHA256_CTX *ctx)
+static void my_sha256_init(my_sha256_ctx *ctx)
 {
   sha256_init(ctx);
 }
 
-static void SHA256_Update(SHA256_CTX *ctx,
-                          const unsigned char *data,
-                          unsigned int length)
+static void my_sha256_update(my_sha256_ctx *ctx,
+                             const unsigned char *data,
+                             unsigned int length)
 {
   sha256_update(ctx, length, data);
 }
 
-static void SHA256_Final(unsigned char *digest, SHA256_CTX *ctx)
+static void my_sha256_final(unsigned char *digest, my_sha256_ctx *ctx)
 {
   sha256_digest(ctx, SHA256_DIGEST_SIZE, digest);
 }
@@ -109,9 +138,9 @@ static void SHA256_Final(unsigned char *digest, SHA256_CTX *ctx)
 /* The last #include file should be: */
 #include "memdebug.h"
 
-typedef mbedtls_sha256_context SHA256_CTX;
+typedef mbedtls_sha256_context my_sha256_ctx;
 
-static void SHA256_Init(SHA256_CTX *ctx)
+static void my_sha256_init(my_sha256_ctx *ctx)
 {
 #if !defined(HAS_MBEDTLS_RESULT_CODE_BASED_FUNCTIONS)
   (void) mbedtls_sha256_starts(ctx, 0);
@@ -120,9 +149,9 @@ static void SHA256_Init(SHA256_CTX *ctx)
 #endif
 }
 
-static void SHA256_Update(SHA256_CTX *ctx,
-                          const unsigned char *data,
-                          unsigned int length)
+static void my_sha256_update(my_sha256_ctx *ctx,
+                             const unsigned char *data,
+                             unsigned int length)
 {
 #if !defined(HAS_MBEDTLS_RESULT_CODE_BASED_FUNCTIONS)
   (void) mbedtls_sha256_update(ctx, data, length);
@@ -131,7 +160,7 @@ static void SHA256_Update(SHA256_CTX *ctx,
 #endif
 }
 
-static void SHA256_Final(unsigned char *digest, SHA256_CTX *ctx)
+static void my_sha256_final(unsigned char *digest, my_sha256_ctx *ctx)
 {
 #if !defined(HAS_MBEDTLS_RESULT_CODE_BASED_FUNCTIONS)
   (void) mbedtls_sha256_finish(ctx, digest);
@@ -152,21 +181,21 @@ static void SHA256_Final(unsigned char *digest, SHA256_CTX *ctx)
 /* The last #include file should be: */
 #include "memdebug.h"
 
-typedef CC_SHA256_CTX SHA256_CTX;
+typedef CC_SHA256_CTX my_sha256_ctx;
 
-static void SHA256_Init(SHA256_CTX *ctx)
+static void my_sha256_init(my_sha256_ctx *ctx)
 {
   (void) CC_SHA256_Init(ctx);
 }
 
-static void SHA256_Update(SHA256_CTX *ctx,
-                          const unsigned char *data,
-                          unsigned int length)
+static void my_sha256_update(my_sha256_ctx *ctx,
+                             const unsigned char *data,
+                             unsigned int length)
 {
   (void) CC_SHA256_Update(ctx, data, length);
 }
 
-static void SHA256_Final(unsigned char *digest, SHA256_CTX *ctx)
+static vo
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-d7e2bf23b7` → 本草稿移入 `cases/defect/auto-curl-d7e2bf23b7/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
