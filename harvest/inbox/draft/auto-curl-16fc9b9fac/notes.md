# auto-curl-16fc9b9fac

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | curl/curl |
| 源 PR | [#10041](https://github.com/curl/curl/pull/10041) |
| 许可证 | MIT |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-08-31 |
| track 方向 | contract 候选（polarity=must_not_find） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #10041 (https://github.com/curl/curl/pull/10041)
- 候选初判 scenario: **None（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 10041 imap: Provide method to disable SASL if it is advertised :: FP 修复 PR（静态分析误报/契约安全），contract 轨误报矿候选（待 LLM/人审定）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1925,6 +1925,7 @@ static CURLcode imap_parse_url_options(struct connectdata *conn)
   CURLcode result = CURLE_OK;
   struct imap_conn *imapc = &conn->proto.imapc;
   const char *ptr = conn->options;
+  bool prefer_login = false;
 
   while(!result && ptr && *ptr) {
     const char *key = ptr;
@@ -1938,26 +1939,39 @@ static CURLcode imap_parse_url_options(struct connectdata *conn)
     while(*ptr && *ptr != ';')
       ptr++;
 
-    if(strncasecompare(key, "AUTH=", 5))
+    if(strncasecompare(key, "AUTH=+LOGIN", 11)) {
+      /* User prefers plaintext LOGIN over any SASL, including SASL LOGIN */
+      prefer_login = true;
+      imapc->sasl.prefmech = SASL_AUTH_NONE;
+    }
+    else if(strncasecompare(key, "AUTH=", 5)) {
+      prefer_login = false;
       result = Curl_sasl_parse_url_auth_option(&imapc->sasl,
                                                value, ptr - value);
-    else
+    }
+    else {
+      prefer_login = false;
       result = CURLE_URL_MALFORMAT;
+    }
 
     if(*ptr == ';')
       ptr++;
   }
 
-  switch(imapc->sasl.prefmech) {
-  case SASL_AUTH_NONE:
-    imapc->preftype = IMAP_TYPE_NONE;
-    break;
-  case SASL_AUTH_DEFAULT:
-    imapc->preftype = IMAP_TYPE_ANY;
-    break;
-  default:
-    imapc->preftype = IMAP_TYPE_SASL;
-    break;
+  if(prefer_login)
+    imapc->preftype = IMAP_TYPE_CLEARTEXT;
+  else {
+    switch(imapc->sasl.prefmech) {
+    case SASL_AUTH_NONE:
+      imapc->preftype = IMAP_TYPE_NONE;
+      break;
+    case SASL_AUTH_DEFAULT:
+      imapc->preftype = IMAP_TYPE_ANY;
+      break;
+    default:
+      imapc->preftype = IMAP_TYPE_SASL;
+      break;
+    }
   }
 
   return result;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`strncasecompare`
- 大写宏：`AUTH`
- 大写宏：`CURLE_OK`
- 大写宏：`CURLE_URL_MALFORMAT`
- 大写宏：`IMAP_TYPE_ANY`
- 大写宏：`IMAP_TYPE_NONE`
- 大写宏：`IMAP_TYPE_SASL`
- 大写宏：`SASL_AUTH_DEFAULT`
- 大写宏：`SASL_AUTH_NONE`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## 为什么契约安全

（模板，移植者填：此处报出为什么是误报？豁免契约是什么？与 contract.yaml 的 contract.name / exemption_pattern 对应。）

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-curl-16fc9b9fac` → 本草稿移入 `cases/contract/auto-curl-16fc9b9fac/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
