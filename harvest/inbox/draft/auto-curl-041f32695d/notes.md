# auto-curl-041f32695d

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #1393 (https://github.com/curl/curl/pull/1393)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 1964；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -56,7 +56,8 @@
 #include <base64.h>
 #include <cert.h>
 #include <prerror.h>
-#include <keyhi.h>        /* for SECKEY_DestroyPublicKey() */
+#include <keyhi.h>         /* for SECKEY_DestroyPublicKey() */
+#include <private/pprio.h> /* for PR_ImportTCPSocket */
 
 #define NSSVERNUM ((NSS_VMAJOR<<16)|(NSS_VMINOR<<8)|NSS_VPATCH)
 
@@ -77,7 +78,6 @@
 /* enough to fit the string "PEM Token #[0|1]" */
 #define SLOTSIZE 13
 
-PRFileDesc *PR_ImportTCPSocket(PRInt32 osfd);
 static PRLock *nss_initlock = NULL;
 static PRLock *nss_crllock = NULL;
 static PRLock *nss_findslot_lock = NULL;
@@ -401,7 +401,7 @@ static CURLcode nss_create_object(struct ssl_connect_data *ssl,
   PK11_SETATTRS(attrs, attr_cnt, CKA_CLASS, &obj_class, sizeof(obj_class));
   PK11_SETATTRS(attrs, attr_cnt, CKA_TOKEN, &cktrue, sizeof(CK_BBOOL));
   PK11_SETATTRS(attrs, attr_cnt, CKA_LABEL, (unsigned char *)filename,
-                strlen(filename) + 1);
+                (CK_ULONG)strlen(filename) + 1);
 
   if(CKO_CERTIFICATE == obj_class) {
     CK_BBOOL *pval = (cacert) ? (&cktrue) : (&ckfalse);
@@ -1960,8 +1960,8 @@ static CURLcode nss_do_connect(struct connectdata *conn, int sockindex)
 
 
   /* check timeout situation */
-  const long time_left = Curl_timeleft(data, NULL, TRUE);
-  if(time_left < 0L) {
+  const time_t time_left = Curl_timeleft(data, NULL, TRUE);
+  if(time_left < 0) {
     failf(data, "timed out before SSL handshake");
     result = CURLE_OPERATION_TIMEDOUT;
     goto error;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-041f32695d` → 本草稿移入 `cases/defect/auto-curl-041f32695d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
