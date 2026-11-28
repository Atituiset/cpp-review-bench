# auto-postgres-52419ca08d

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#e9526740926a801554cc92ea4d5620fca3546fe8](https://github.com/postgres/postgres/commit/e9526740926a801554cc92ea4d5620fca3546fe8) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 101 |
| 编译错误数（gcc syntax-only） | 103（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #e9526740926a801554cc92ea4d5620fca3546fe8 (https://github.com/postgres/postgres/commit/e9526740926a801554cc92ea4d5620fca3546fe8)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 9（原始 PR diff 行 1604；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT e9526740926a801554cc92ea4d5620fca3546fe8 Let OpenSSL auto-select DH parameters by default :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -83,7 +83,6 @@ static BIO_METHOD *port_bio_method(void);
 static int	ssl_set_port_bio(Port *port);
 
 static DH  *load_dh_file(char *filename, bool isServerStart);
-static DH  *load_dh_buffer(const char *buffer, size_t len);
 static int	ssl_external_passwd_cb(char *buf, int size, int rwflag, void *userdata);
 static int	dummy_ssl_passwd_cb(char *buf, int size, int rwflag, void *userdata);
 static int	verify_cb(int ok, X509_STORE_CTX *ctx);
@@ -1587,32 +1586,6 @@ load_dh_file(char *filename, bool isServerStart)
 	return dh;
 }
 
-/*
- *	Load hardcoded DH parameters.
- *
- *	If DH parameters cannot be loaded from a specified file, we can load
- *	the hardcoded DH parameters supplied with the backend to prevent
- *	problems.
- */
-static DH  *
-load_dh_buffer(const char *buffer, size_t len)
-{
-	BIO		   *bio;
-	DH		   *dh = NULL;
-
-	bio = BIO_new_mem_buf(buffer, len);
-	if (bio == NULL)
-		return NULL;
-	dh = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
-	if (dh == NULL)
-		ereport(DEBUG2,
-				(errmsg_internal("DH load buffer: %s",
-								 SSLerrmessage(ERR_get_error()))));
-	BIO_free(bio);
-
-	return dh;
-}
-
 /*
  *	Passphrase collection callback using ssl_passphrase_command
  */
@@ -2134,48 +2107,48 @@ sni_clienthello_cb(SSL *ssl, int *al, void *arg)
 #endif							/* HAVE_SSL_CTX_SET_CLIENT_HELLO_CB */
 
 /*
- * Set DH parameters for generating ephemeral DH keys.  The
- * DH parameters can take a long time to compute, so they must be
- * precomputed.
- *
- * Since few sites will bother to create a parameter file, we also
- * provide a fallback to the parameters provided by the OpenSSL
- * project.
- *
- * These values can be static (once loaded or computed) since the
- * OpenSSL library can efficiently generate random keys from the
- * information provided.
+ * Set DH parameters for generating ephemeral DH keys.
  */
 static bool
 initialize_dh(SSL_CTX *context, bool isServerStart)
 {
-	DH		   *dh = NULL;
-
 	SSL_CTX_set_options(context, SSL_OP_SINGLE_DH_USE);
 
 	if (ssl_dh_params_file[0])
-		dh = load_dh_file(ssl_dh_params_file, isServerStart);
-	if (!dh)
-		dh = load_dh_buffer(FILE_DH2048, sizeof(FILE_DH2048));
-	if (!dh)
 	{
-		ereport(isServerStart ? FATAL : LOG,
-				(errcode(ERRCODE_CONFIG_FILE_ERROR),
-				 errmsg("DH: could not load DH parameters")));
-		return false;
-	}
+		DH		   *dh;
+
+		dh = load_dh_file(ssl_dh_params_file, isServerStart);
+
+		if (!dh)
+			return false;
+
+		if (SSL_CTX_set_tmp_dh(context, dh) != 1)
+		{
+			ereport(isServerStart ? FATAL : LOG,
+					(errcode(ERRCODE_CONFIG_FILE_ERROR),
+					 errmsg("DH: could not set DH parameters: %s",
+							SSLerrmessage(ERR_get_error()))));
+			DH_free(dh);
+			return false;
+		}
 
-	if (SSL_CTX_set_tmp_dh(context, dh) != 1)
-	{
-		ereport(isServerStart ? FATAL : LOG,
-				(errcode(ERRCODE_CONFIG_FILE_ERROR),
-				 errmsg("DH: could not set DH parameters: %s",
-						SSLerrmessage(ERR_get_error()))));
 		DH_free(dh);
-		return false;
+	}
+	else
+	{
+		/* If ssl_dh_params_file is not set, let OpenSSL pick a default. */
+
+		if (SSL_CTX_set_dh_auto(context, 1) != 1)
+		{
+			ereport(isServerStart ? FATAL : LOG,
+					(errcode(ERRCODE_INTERNAL_ERROR),
+					 errmsg("DH: could not set DH parameters: %s",
+							SSLerrmessage(ERR_get_error()))));
+			return false;
+		}
 	}
 
-	DH_free(dh);
 	return true;
 }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errcode_for_file_access`
- 外部函数：`errmsg`
- 外部函数：`errmsg_internal`
- 外部函数：`run_ssl_passphrase_command`
- 外部函数：`secure_raw_read`
- 外部函数：`secure_raw_write`
- 外部函数：`strlen`
- 外部函数：`verify_cb`
- 大写宏：`BIO`
- 大写宏：`BIO_CTRL_EOF`
- 大写宏：`BIO_CTRL_FLUSH`
- 大写宏：`BIO_METHOD`
- 大写宏：`BIO_TYPE_SOURCE_SINK`
- 大写宏：`DEBUG2`
- 大写宏：`DH_CHECK_P_NOT_PRIME`
- 大写宏：`DH_CHECK_P_NOT_SAFE_PRIME`
- 大写宏：`DH_NOT_SUITABLE_GENERATOR`
- 大写宏：`EAGAIN`
- 大写宏：`EINTR`
- 大写宏：`EOF`
- 大写宏：`ERRCODE_CONFIG_FILE_ERROR`
- 大写宏：`EWOULDBLOCK`
- 大写宏：`FATAL`
- 大写宏：`FILE`
- 大写宏：`LOG`
- 大写宏：`NULL`
- 大写宏：`PEM`
- 大写宏：`X509_STORE_CTX`
- 外部类型：`And`
- 外部类型：`BIO_ctrl`
- 外部类型：`BIO_flush`
- 外部类型：`BIOs`
- 外部类型：`Common`
- 外部类型：`Enter`
- 外部类型：`However`
- 外部类型：`If`
- 外部类型：`It`
- 外部类型：`Load`
- 外部类型：`Names`
- 外部类型：`OpenSSL`
- 外部类型：`Passphrase`
- 外部类型：`Port`
- 外部类型：`PostgreSQL`
- 外部类型：`See`
- 外部类型：`Set`
- 外部类型：`This`
- 外部类型：`size_t`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-52419ca08d` → 本草稿移入 `cases/defect/auto-postgres-52419ca08d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
