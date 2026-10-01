# auto-curl-e03b987005

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #10917 (https://github.com/curl/curl/pull/10917)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 283；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -65,6 +65,17 @@ struct gss_ctx_id_t_desc_struct {
   char creds[MAX_CREDS_LENGTH];
 };
 
+/* simple implementation of strndup(), which isn't portable */
+static char *my_strndup(const char *ptr, size_t len)
+{
+  char *copy = malloc(len + 1);
+  if(!copy)
+    return NULL;
+  memcpy(copy, ptr, len);
+  copy[len] = '\0';
+  return copy;
+}
+
 OM_uint32 gss_init_sec_context(OM_uint32 *min,
             gss_const_cred_id_t initiator_cred_handle,
             gss_ctx_id_t *context_handle,
@@ -280,7 +291,7 @@ OM_uint32 gss_import_name(OM_uint32 *min,
     return GSS_S_FAILURE;
   }
 
-  name = strndup(input_name_buffer->value, input_name_buffer->length);
+  name = my_strndup(input_name_buffer->value, input_name_buffer->length);
   if(!name) {
     *min = GSS_NO_MEMORY;
     return GSS_S_FAILURE;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-e03b987005` → 本草稿移入 `cases/defect/auto-curl-e03b987005/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
