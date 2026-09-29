# auto-nginx-f2a996d380

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | nginx/nginx |
| 源 PR | [#1769](https://github.com/nginx/nginx/pull/1769) |
| 许可证 | BSD-2-Clause |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-29 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 48 |
| 编译错误数（gcc syntax-only） | 7（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1769 (https://github.com/nginx/nginx/pull/1769)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 73；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 1769 QUIC compatibility layer fix :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -70,11 +70,16 @@ static ngx_int_t ngx_quic_compat_create_record(ngx_quic_compat_record_t *rec,
     ngx_str_t *res);
 
 
-ngx_int_t
-ngx_quic_compat_init(ngx_conf_t *cf, SSL_CTX *ctx)
+void
+ngx_quic_compat_keylog_init(SSL_CTX *ctx)
 {
     SSL_CTX_set_keylog_callback(ctx, ngx_quic_compat_keylog_callback);
+}
+
 
+ngx_int_t
+ngx_quic_compat_ext_init(ngx_conf_t *cf, SSL_CTX *ctx)
+{
     if (SSL_CTX_has_client_custom_ext(ctx, NGX_QUIC_COMPAT_SSL_TP_EXT)) {
         return NGX_OK;
     }
@@ -341,7 +346,7 @@ ngx_quic_compat_parse_transport_params_callback(SSL *ssl, unsigned int ext_type,
 
     c = ngx_ssl_get_connection(ssl);
     if (c->type != SOCK_DGRAM) {
-        return 0;
+        return 1;
     }
 
     ngx_log_debug0(NGX_LOG_DEBUG_EVENT, c->log, 0,
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`ngx_explicit_memzero`
- 外部函数：`ngx_log_debug2`
- 外部函数：`ngx_log_error`
- 外部函数：`ngx_pool_cleanup_add`
- 外部函数：`ngx_quic_ciphers`
- 外部函数：`ngx_quic_crypto_cleanup`
- 外部函数：`ngx_quic_crypto_init`
- 外部函数：`ngx_quic_get_connection`
- 外部函数：`ngx_quic_hkdf_expand`
- 外部函数：`ngx_quic_hkdf_set`
- 外部函数：`ngx_ssl_error`
- 外部函数：`ngx_ssl_get_connection`
- 外部函数：`ngx_strncmp`
- 外部函数：`set_read_secret`
- 外部函数：`set_write_secret`
- 大写宏：`CLIENT_HANDSHAKE_TRAFFIC_SECRET`
- 大写宏：`CLIENT_TRAFFIC_SECRET_0`
- 大写宏：`EVP_MAX_MD_SIZE`
- 大写宏：`NGX_ERROR`
- 大写宏：`NGX_LOG_DEBUG_EVENT`
- 大写宏：`NGX_LOG_EMERG`
- 大写宏：`NGX_LOG_INFO`
- 大写宏：`NGX_OK`
- 大写宏：`NGX_QUIC_ERR_INTERNAL_ERROR`
- 大写宏：`NGX_QUIC_IV_LEN`
- 大写宏：`NULL`
- 大写宏：`QUIC`
- 大写宏：`SERVER_HANDSHAKE_TRAFFIC_SECRET`
- 大写宏：`SERVER_TRAFFIC_SECRET_0`
- 大写宏：`SOCK_DGRAM`
- 大写宏：`SSL`
- 大写宏：`SSL_CIPHER`
- 大写宏：`SSL_CTX`
- 外部类型：`OpenSSL`
- 外部类型：`ngx_conf_t`
- 外部类型：`ngx_connection_t`
- 外部类型：`ngx_int_t`
- 外部类型：`ngx_pool_cleanup_t`
- 外部类型：`ngx_quic_ciphers_t`
- 外部类型：`ngx_quic_compat_keys_t`
- 外部类型：`ngx_quic_compat_t`
- 外部类型：`ngx_quic_connection_t`
- 外部类型：`ngx_quic_hkdf_t`
- 外部类型：`ngx_quic_md_t`
- 外部类型：`ngx_quic_secret_t`
- 外部类型：`ngx_str_t`
- 外部类型：`ngx_uint_t`
- 外部类型：`size_t`
- 外部类型：`ssl_encryption_level_t`
- 外部类型：`uint8_t`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。
- **依赖重（dep_count≥10）**：可考虑只做 PR/diff 形态评审，不做独立 case。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-nginx-f2a996d380` → 本草稿移入 `cases/defect/auto-nginx-f2a996d380/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
