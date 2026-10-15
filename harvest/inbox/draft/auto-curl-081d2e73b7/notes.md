# auto-curl-081d2e73b7

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | curl/curl |
| 源 PR | [#20545](https://github.com/curl/curl/pull/20545) |
| 许可证 | MIT |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-08-31 |
| track 方向 | defect 候选（polarity=must_find） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #20545 (https://github.com/curl/curl/pull/20545)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 471；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 20545 digest: escape double quotes and backslashes in the realm :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -354,6 +354,8 @@ CURLcode Curl_auth_create_digest_md5_message(struct Curl_easy *data,
   char method[]     = "AUTHENTICATE";
   char qop[]        = DIGEST_QOP_VALUE_STRING_AUTH;
   char *spn         = NULL;
+  char *qrealm;
+  char *qnonce;
 
   /* Decode the challenge message */
   CURLcode result = auth_decode_digest_md5_message(chlg,
@@ -467,12 +469,20 @@ CURLcode Curl_auth_create_digest_md5_message(struct Curl_easy *data,
   for(i = 0; i < MD5_DIGEST_LEN; i++)
     curl_msnprintf(&resp_hash_hex[2 * i], 3, "%02x", digest[i]);
 
-  /* Generate the response */
-  response = curl_maprintf("username=\"%s\",realm=\"%s\",nonce=\"%s\","
-                           "cnonce=\"%s\",nc=\"%s\",digest-uri=\"%s\","
-                           "response=%s,qop=%s",
-                           userp, realm, nonce,
-                           cnonce, nonceCount, spn, resp_hash_hex, qop);
+  /* escape double quotes and backslashes in the realm and nonce as
+     necessary */
+  qrealm = auth_digest_string_quoted(realm);
+  qnonce = auth_digest_string_quoted(nonce);
+  if(qrealm && qnonce)
+    /* Generate the response */
+    response = curl_maprintf("username=\"%s\",realm=\"%s\",nonce=\"%s\","
+                             "cnonce=\"%s\",nc=\"%s\",digest-uri=\"%s\","
+                             "response=%s,qop=%s",
+                             userp, qrealm, qnonce,
+                             cnonce, nonceCount, spn, resp_hash_hex, qop);
+
+  curlx_free(qrealm);
+  curlx_free(qnonce);
   curlx_free(spn);
   if(!response)
     return CURLE_OUT_OF_MEMORY;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`auth_decode_digest_md5_message`
- 外部函数：`curl_maprintf`
- 外部函数：`curl_msnprintf`
- 外部函数：`curlx_free`
- 大写宏：`AUTHENTICATE`
- 大写宏：`CURLE_OUT_OF_MEMORY`
- 大写宏：`DIGEST_QOP_VALUE_STRING_AUTH`
- 大写宏：`MD5_DIGEST_LEN`
- 大写宏：`NULL`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-curl-081d2e73b7` → 本草稿移入 `cases/defect/auto-curl-081d2e73b7/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
