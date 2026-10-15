# auto-curl-5d1fcc43b6

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #bc440a89d47aa8f3a5d02b01985fd7f6fb7e7e0a (https://github.com/curl/curl/commit/bc440a89d47aa8f3a5d02b01985fd7f6fb7e7e0a)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -419,13 +419,22 @@ static size_t encoder_base64_read(char *buffer, size_t size, bool ateof,
   return cursize;
 }
 
+/* The maximum input size that does not cause an overflow. */
+#define BASE64_MAX_INPUT_SIZE                                           \
+  (((CURL_OFF_T_MAX / (MAX_ENCODED_LINE_LENGTH + 2)) *                  \
+    MAX_ENCODED_LINE_LENGTH / 4) * 3 - 3)
+
 static curl_off_t encoder_base64_size(curl_mimepart *part)
 {
   curl_off_t size = part->datasize;
 
   if(size <= 0)
     return size;    /* Unknown size or no data. */
 
+  /* Prevent integer overflows */
+  if(size > BASE64_MAX_INPUT_SIZE)
+    return -1;
+
   /* Compute base64 character count. */
   size = 4 * (1 + ((size - 1) / 3));
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-5d1fcc43b6` → 本草稿移入 `cases/defect/auto-curl-5d1fcc43b6/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
