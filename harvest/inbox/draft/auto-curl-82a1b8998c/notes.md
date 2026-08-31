# auto-curl-82a1b8998c

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #7808 (https://github.com/curl/curl/pull/7808)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -49,7 +49,14 @@
      in NTLM type-3 messages.
  */
 
-#if defined(USE_OPENSSL) || defined(USE_WOLFSSL)
+#if defined(USE_OPENSSL)
+  #include <openssl/opensslconf.h>
+  #if !defined(OPENSSL_NO_DES) && !defined(OPENSSL_NO_DEPRECATED_3_0)
+    #define USE_OPENSSL_DES
+  #endif
+#endif
+
+#if defined(USE_OPENSSL_DES) || defined(USE_WOLFSSL)
 
 #ifdef USE_WOLFSSL
 #include <wolfssl/options.h>
@@ -97,7 +104,7 @@
 #elif defined(USE_WIN32_CRYPTO)
 #  include <wincrypt.h>
 #else
-#  error "Can't compile NTLM support without a crypto library."
+#  error "Can't compile NTLM support without a crypto library with DES."
 #endif
 
 #include "urldata.h"
@@ -133,7 +140,7 @@ static void extend_key_56_to_64(const unsigned char *key_56, char *key)
   key[7] = (unsigned char) ((key_56[6] << 1) & 0xFF);
 }
 
-#if defined(USE_OPENSSL) || defined(USE_WOLFSSL)
+#if defined(USE_OPENSSL_DES) || defined(USE_WOLFSSL)
 /*
  * Turns a 56 bit key into the 64 bit, odd parity key and sets the key.  The
  * key schedule ks is also set.
@@ -362,7 +369,7 @@ void Curl_ntlm_core_lm_resp(const unsigned char *keys,
                             const unsigned char *plaintext,
                             unsigned char *results)
 {
-#if defined(USE_OPENSSL) || defined(USE_WOLFSSL)
+#if defined(USE_OPENSSL_DES) || defined(USE_WOLFSSL)
   DES_key_schedule ks;
 
   setup_des_key(keys, DESKEY(ks));
@@ -420,7 +427,7 @@ CURLcode Curl_ntlm_core_mk_lm_hash(struct Curl_easy *data,
   {
     /* Create LanManager hashed password. */
 
-#if defined(USE_OPENSSL) || defined(USE_WOLFSSL)
+#if defined(USE_OPENSSL_DES) || defined(USE_WOLFSSL)
     DES_key_schedule ks;
 
     setup_des_key(pw, DESKEY(ks));
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-82a1b8998c` → 本草稿移入 `cases/defect/auto-curl-82a1b8998c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
