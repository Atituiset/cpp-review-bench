# auto-nginx-ee310bb2ed

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #631 (https://github.com/nginx/nginx/pull/631)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 128；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -125,9 +125,10 @@ ngx_quic_keys_set_initial_secret(ngx_quic_keys_t *keys, ngx_str_t *secret,
     ngx_quic_secret_t   *client, *server;
     ngx_quic_ciphers_t   ciphers;
 
-    static const uint8_t salt[20] =
-        "\x38\x76\x2c\xf7\xf5\x59\x34\xb3\x4d\x17"
-        "\x9a\xe6\xa4\xc8\x0c\xad\xcc\xbb\x7f\x0a";
+    static const uint8_t salt[20] = {
+        0x38, 0x76, 0x2c, 0xf7, 0xf5, 0x59, 0x34, 0xb3, 0x4d, 0x17,
+        0x9a, 0xe6, 0xa4, 0xc8, 0x0c, 0xad, 0xcc, 0xbb, 0x7f, 0x0a
+    };
 
     client = &keys->secrets[ssl_encryption_initial].client;
     server = &keys->secrets[ssl_encryption_initial].server;
@@ -958,8 +959,9 @@ ngx_quic_create_retry_packet(ngx_quic_header_t *pkt, ngx_str_t *res)
     /* 5.8.  Retry Packet Integrity */
     static ngx_quic_md_t  key = ngx_quic_md(
         "\xbe\x0c\x69\x0b\x9f\x66\x57\x5a\x1d\x76\x6b\x54\xe3\x68\xc8\x4e");
-    static const u_char   nonce[NGX_QUIC_IV_LEN] =
-        "\x46\x15\x99\xd3\x5d\x63\x2b\xf2\x23\x98\x25\xbb";
+    static const u_char   nonce[NGX_QUIC_IV_LEN] = {
+        0x46, 0x15, 0x99, 0xd3, 0x5d, 0x63, 0x2b, 0xf2, 0x23, 0x98, 0x25, 0xbb
+    };
     static ngx_str_t      in = ngx_string("");
 
     ad.data = res->data;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-ee310bb2ed` → 本草稿移入 `cases/defect/auto-nginx-ee310bb2ed/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
