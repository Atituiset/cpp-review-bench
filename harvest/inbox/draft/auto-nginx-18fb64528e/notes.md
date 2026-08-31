# auto-nginx-18fb64528e

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1204 (https://github.com/nginx/nginx/pull/1204)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -15,6 +15,7 @@
 
 extern uint32_t  *ngx_crc32_table_short;
 extern uint32_t   ngx_crc32_table256[];
+extern uint32_t   ngx_crc32c_table256[];
 
 
 static ngx_inline uint32_t
@@ -73,6 +74,44 @@ ngx_crc32_update(uint32_t *crc, u_char *p, size_t len)
     crc ^= 0xffffffff
 
 
+static ngx_inline uint32_t
+ngx_crc32c_long(u_char *p, size_t len)
+{
+    uint32_t  crc;
+
+    crc = 0xffffffff;
+
+    while (len--) {
+        crc = ngx_crc32c_table256[(crc ^ *p++) & 0xff] ^ (crc >> 8);
+    }
+
+    return crc ^ 0xffffffff;
+}
+
+
+#define ngx_crc32c_init(crc)                                                  \
+    crc = 0xffffffff
+
+
+static ngx_inline void
+ngx_crc32c_update(uint32_t *crc, u_char *p, size_t len)
+{
+    uint32_t  c;
+
+    c = *crc;
+
+    while (len--) {
+        c = ngx_crc32c_table256[(c ^ *p++) & 0xff] ^ (c >> 8);
+    }
+
+    *crc = c;
+}
+
+
+#define ngx_crc32c_final(crc)                                                 \
+    crc ^= 0xffffffff
+
+
 ngx_int_t ngx_crc32_table_init(void);
 
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-18fb64528e` → 本草稿移入 `cases/defect/auto-nginx-18fb64528e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
