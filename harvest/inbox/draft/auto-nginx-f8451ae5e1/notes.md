# auto-nginx-f8451ae5e1

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1204 (https://github.com/nginx/nginx/pull/1204)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 106；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -9,9 +9,35 @@
 #include <ngx_core.h>
 
 
+#define NGX_PROXY_PROTOCOL_V2_MAX_HEADER    52
+
+#define NGX_PROXY_PROTOCOL_CMD_LOCAL        0
+#define NGX_PROXY_PROTOCOL_CMD_PROXY        1
+
+#define NGX_PROXY_PROTOCOL_AF_UNSPEC        0
 #define NGX_PROXY_PROTOCOL_AF_INET          1
 #define NGX_PROXY_PROTOCOL_AF_INET6         2
 
+#define NGX_PROXY_PROTOCOL_TYPE_UNSPEC      0
+#define NGX_PROXY_PROTOCOL_TYPE_STREAM      1
+#define NGX_PROXY_PROTOCOL_TYPE_DGRAM       2
+
+#define NGX_PROXY_PROTOCOL_TLV_ALPN         0x01
+#define NGX_PROXY_PROTOCOL_TLV_AUTHORITY    0x02
+#define NGX_PROXY_PROTOCOL_TLV_CRC32C       0x03
+#define NGX_PROXY_PROTOCOL_TLV_UNIQUE_ID    0x05
+#define NGX_PROXY_PROTOCOL_TLV_SSL          0x20
+#define NGX_PROXY_PROTOCOL_TLV_SSL_VERSION  0x21
+#define NGX_PROXY_PROTOCOL_TLV_SSL_CN       0x22
+#define NGX_PROXY_PROTOCOL_TLV_SSL_CIPHER   0x23
+#define NGX_PROXY_PROTOCOL_TLV_SSL_SIG_ALG  0x24
+#define NGX_PROXY_PROTOCOL_TLV_SSL_KEY_ALG  0x25
+#define NGX_PROXY_PROTOCOL_TLV_NETNS        0x30
+
+#define NGX_PROXY_PROTOCOL_V2_CLIENT_SSL          0x01
+#define NGX_PROXY_PROTOCOL_V2_CLIENT_CERT_CONN    0x02
+#define NGX_PROXY_PROTOCOL_V2_CLIENT_CERT_SESS    0x04
+
 
 #define ngx_proxy_protocol_parse_uint16(p)                                    \
     ( ((uint16_t) (p)[0] << 8)                                                \
@@ -66,6 +92,12 @@ typedef struct {
 } ngx_proxy_protocol_tlv_entry_t;
 
 
+typedef struct {
+    ngx_uint_t                              type;
+    ngx_str_t                               value;
+} ngx_proxy_protocol_tlv_value_t;
+
+
 static u_char *ngx_proxy_protocol_read_addr(ngx_connection_t *c, u_char *p,
     u_char *last, ngx_str_t *addr);
 static u_char *ngx_proxy_protocol_read_port(u_char *p, u_char *last,
@@ -74,42 +106,70 @@ static u_char *ngx_proxy_protocol_v2_read(ngx_connection_t *c, u_char *buf,
     u_char *last);
 static ngx_int_t ngx_proxy_protocol_lookup_tlv(ngx_connection_t *c,
     ngx_str_t *tlvs, ngx_uint_t type, ngx_str_t *value);
+#if (NGX_OPENSSL)
+static ngx_int_t ngx_proxy_protocol_v2_eval_ssl(ngx_connection_t *c,
+    ngx_array_t *tlvs, ngx_array_t *ssl_tlvs, ngx_uint_t *client,
+    uint32_t *verify);
+static ngx_int_t ngx_proxy_protocol_v2_authority(ngx_connection_t *c,
+    ngx_str_t *out);
+static ngx_int_t ngx_proxy_protocol_v2_alpn(ngx_connection_t *c,
+    ngx_str_t *out);
+static ngx_int_t ngx_proxy_protocol_v2_ssl_sub(ngx_connection_t *c,
+    ngx_uint_t type, ngx_str_t *out);
+#endif
+static u_char *ngx_proxy_protocol_v2_write_header(ngx_connection_t *c,
+    u_char *buf, u_char *last);
+static u_char ngx_proxy_protocol_v2_family(ngx_uint_t family);
+static void ngx_proxy_protocol_v2_write_ipv4(struct sockaddr *sa, u_char *addr,
+    u_char *port);
+static void ngx_proxy_protocol_v2_write_ipv6(struct sockaddr *sa, u_char *addr,
+    u_char *port);
+static u_char *ngx_proxy_protocol_v2_write_tlv(ngx_connection_t *c, u_char *p,
+    u_char *last, ngx_uint_t type, ngx_str_t *value);
+static u_char *ngx_proxy_protocol_v2_write_ssl(ngx_connection_t *c, u_char *p,
+    u_char *last, ngx_array_t *ssl_tlvs, ngx_uint_t client, uint32_t verify);
+static void ngx_proxy_protocol_v2_set_len(u_char *buf, u_char *p);
+static u_char *ngx_proxy_protocol_v2_write_crc32c(ngx_connection_t *c,
+    u_char *buf, u_char *p, u_char *last);
 
 
 static ngx_proxy_protocol_tlv_entry_t  ngx_proxy_protocol_tlv_entries[] = {
-    { ngx_string("alpn"),       0x01 },
-    { ngx_string("authority"),  0x02 },
-    { ngx_string("unique_id"),  0x05 },
-    { ngx_string("ssl"),        0x20 },
-    { ngx_string("netns"),      0x30 },
+    { ngx_string("alpn"),       NGX_PROXY_PROTOCOL_TLV_ALPN },
+    { ngx_string("authority"),  NGX_PROXY_PROTOCOL_TLV_AUTHORITY },
+    { ngx_string("unique_id"),  NGX_PROXY_PROTOCOL_TLV_UNIQUE_ID },
+    { ngx_string("ssl"),        NGX_PROXY_PROTOCOL_TLV_SSL },
+    { ngx_string("netns"),      NGX_PROXY_PROTOCOL_TLV_NETNS },
     { ngx_null_string,          
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-f8451ae5e1` → 本草稿移入 `cases/defect/auto-nginx-f8451ae5e1/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
