# auto-postgres-06f2de8ca2

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#73401447c3404aa8bebf8a4c89d9e604dc6797c4](https://github.com/postgres/postgres/commit/73401447c3404aa8bebf8a4c89d9e604dc6797c4) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-26 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 15 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #73401447c3404aa8bebf8a4c89d9e604dc6797c4 (https://github.com/postgres/postgres/commit/73401447c3404aa8bebf8a4c89d9e604dc6797c4)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 73401447c3404aa8bebf8a4c89d9e604dc6797c4 Remove backwards compatibility ifdef for SSL_AD_NO_APPLICATION_PROTOCOL :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1642,20 +1642,16 @@ SSLerrmessage(unsigned long ecode)
 
 	/*
 	 * Server aborted the connection with TLS "no_application_protocol" alert.
-	 * The ERR_reason_error_string() function doesn't give any error string
-	 * for that for some reason, so do it ourselves.  See
-	 * https://github.com/openssl/openssl/issues/24300.  This is available in
-	 * OpenSSL 1.1.0 and later, as well as in LibreSSL 3.4.3 (OpenBSD 7.0) and
-	 * later.
+	 * Older OpenSSL and LibreSSL versions don't give any error string for
+	 * that, so do it ourselves.  This is fixed in OpenSSL 3.4 and in LibreSSL
+	 * 4.1.0.  See https://github.com/openssl/openssl/issues/24300.
 	 */
-#ifdef SSL_AD_NO_APPLICATION_PROTOCOL
 	if (ERR_GET_LIB(ecode) == ERR_LIB_SSL &&
 		ERR_GET_REASON(ecode) == SSL_AD_REASON_OFFSET + SSL_AD_NO_APPLICATION_PROTOCOL)
 	{
 		snprintf(errbuf, SSL_ERR_LEN, "no application protocol");
 		return errbuf;
 	}
-#endif
 
 	/*
 	 * In OpenSSL 3.0.0 and later, ERR_reason_error_string does not map system
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`snprintf`
- 大写宏：`ERR_GET_LIB`
- 大写宏：`ERR_GET_REASON`
- 大写宏：`ERR_LIB_SSL`
- 大写宏：`SSL_AD_NO_APPLICATION_PROTOCOL`
- 大写宏：`SSL_AD_REASON_OFFSET`
- 大写宏：`TLS`
- 外部类型：`ERR_reason_error_string`
- 外部类型：`In`
- 外部类型：`LibreSSL`
- 外部类型：`OpenBSD`
- 外部类型：`OpenSSL`
- 外部类型：`See`
- 外部类型：`Server`
- 外部类型：`The`
- 外部类型：`This`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-06f2de8ca2` → 本草稿移入 `cases/defect/auto-postgres-06f2de8ca2/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
