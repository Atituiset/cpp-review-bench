# auto-curl-be642cb626

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #1848 (https://github.com/curl/curl/pull/1848)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 201；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -31,6 +31,25 @@
  * https://www.innovation.ch/java/ntlm.html
  */
 
+/* Please keep the SSL backend-specific #if branches in this order:
+
+   1. USE_OPENSSL
+   2. USE_GNUTLS_NETTLE
+   3. USE_GNUTLS
+   4. USE_NSS
+   5. USE_MBEDTLS
+   6. USE_DARWINSSL
+   7. USE_OS400CRYPTO
+   8. USE_WIN32_CRYPTO
+
+   This ensures that:
+   - the same SSL branch gets activated throughout this source
+     file even if multiple backends are enabled at the same time.
+   - OpenSSL and NSS have higher priority than Windows Crypt, due
+     to issues with the latter supporting NTLM2Session responses
+     in NTLM type-3 messages.
+ */
+
 #if !defined(USE_WINDOWS_SSPI) || defined(USE_WIN32_CRYPTO)
 
 #ifdef USE_OPENSSL
@@ -76,14 +95,6 @@
 #  define MD5_DIGEST_LENGTH 16
 #  define MD4_DIGEST_LENGTH 16
 
-#elif defined(USE_MBEDTLS)
-
-#  include <mbedtls/des.h>
-#  include <mbedtls/md4.h>
-#  if !defined(MBEDTLS_MD4_C)
-#    include "curl_md4.h"
-#  endif
-
 #elif defined(USE_NSS)
 
 #  include <nss.h>
@@ -92,6 +103,14 @@
 #  include "curl_md4.h"
 #  define MD5_DIGEST_LENGTH MD5_LENGTH
 
+#elif defined(USE_MBEDTLS)
+
+#  include <mbedtls/des.h>
+#  include <mbedtls/md4.h>
+#  if !defined(MBEDTLS_MD4_C)
+#    include "curl_md4.h"
+#  endif
+
 #elif defined(USE_DARWINSSL)
 
 #  include <CommonCrypto/CommonCryptor.h>
@@ -196,26 +215,6 @@ static void setup_des_key(const unsigned char *key_56,
   gcry_cipher_setkey(*des, key, sizeof(key));
 }
 
-#elif defined(USE_MBEDTLS)
-
-static bool encrypt_des(const unsigned char *in, unsigned char *out,
-                        const unsigned char *key_56)
-{
-  mbedtls_des_context ctx;
-  char key[8];
-
-  /* Expand the 56-bit key to 64-bits */
-  extend_key_56_to_64(key_56, key);
-
-  /* Set the key parity to odd */
-  mbedtls_des_key_set_parity((unsigned char *) key);
-
-  /* Perform the encryption */
-  mbedtls_des_init(&ctx);
-  mbedtls_des_setkey_enc(&ctx, (unsigned char *) key);
-  return mbedtls_des_crypt_ecb(&ctx, in, out) == 0;
-}
-
 #elif defined(USE_NSS)
 
 /*
@@ -281,6 +280,26 @@ static bool encrypt_des(const unsigned char *in, unsigned char *out,
   return rv;
 }
 
+#elif defined(USE_MBEDTLS)
+
+static bool encrypt_des(const unsigned char *in, unsigned char *out,
+                        const unsigned char *key_56)
+{
+  mbedtls_des_context ctx;
+  char key[8];
+
+  /* Expand the 56-bit key to 64-bits */
+  extend_key_56_to_64(key_56, key);
+
+  /* Set the key parity to odd */
+  mbedtls_des_key_set_parity((unsigned char *) key);
+
+  /* Perform the encryption */
+  mbedtls_des_init(&ctx);
+  mbedtls_des_setkey_enc(&ctx, (unsigned char *) key);
+  return mbedtls_des_crypt_ecb(&ctx, in, out) == 0;
+}
+
 #elif defined(USE_DARWINSSL)
 
 static bool encrypt_des(const unsigned char *in, unsigned char *out,
@@ -428,7 +447,7 @@ void Curl_ntlm_core_lm_resp(const unsigned char *keys,
   setup_des_key(keys + 14, &des);
   gcry_cipher_encrypt(des, results + 16, 8, plaintext, 8);
   gcry_cipher_close(des);
-#elif defined(USE_MBEDTLS) || defined(USE_NSS) || defined(USE_DARWINSSL) \
+#elif defined(USE_NSS) || defined(USE_MBEDTLS) || defined(USE_DARWINSSL) \
   || defined(USE_OS400CRYPTO) || defined(USE_WIN32_CRYPTO)
   encrypt_des(plaintext, results, keys);
   encrypt_des(plaintext, results + 8, keys + 7);
@@ -492,7 +511,7 @@ CURLcode Curl_ntlm_core_mk_lm_hash(struct Curl_easy *data,
     setup_des_key(pw + 7, &des);
     gcry_cipher_encrypt(des, lmbuffer + 8, 8, magic, 8);
     gcry_cipher_close(des);
-#elif defined(USE_MBEDTLS) || defined(USE_NSS) || defined(USE_DARWINSSL) \
+#elif defined(USE_NSS) || defined(USE_MBEDTLS) || defined(USE_DARWINSSL) \
   || defined(USE_OS400CRYPTO) || defined(USE_WIN32_CRYPTO)
     encrypt_des(magic, lmbuffer, pw);
     encrypt_des(magic, lmbuffer + 8, pw + 7);
@@ -571,13 +590,18 @@ CURLcode Curl_ntlm_core_mk_nt_hash(struct Curl_easy *data,
     gcry_md_write(MD4pw, pw, 2 * len);
     memcpy(ntbuffer, gcry_md_read(MD4pw, 0), MD4_DIGEST_LENGTH);
     gcry_md_close(MD4pw);
-
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-be642cb626` → 本草稿移入 `cases/defect/auto-curl-be642cb626/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
