# auto-nginx-acbcbdff7c

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #215 (https://github.com/nginx/nginx/pull/215)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -7,38 +7,42 @@
 
 
 static ngx_bpf_reloc_t bpf_reloc_prog_ngx_quic_reuseport_helper[] = {
-    { "ngx_quic_sockmap", 55 },
+    { "ngx_quic_sockmap", 59 },
 };
 
 static struct bpf_insn bpf_insn_prog_ngx_quic_reuseport_helper[] = {
     /* opcode dst          src         offset imm */
-    { 0x79,   BPF_REG_4,   BPF_REG_1, (int16_t)      0,        0x0 },
+    { 0x79,   BPF_REG_2,   BPF_REG_1, (int16_t)      0,        0x0 },
     { 0x79,   BPF_REG_3,   BPF_REG_1, (int16_t)      8,        0x0 },
-    { 0xbf,   BPF_REG_2,   BPF_REG_4, (int16_t)      0,        0x0 },
-    {  0x7,   BPF_REG_2,   BPF_REG_0, (int16_t)      0,        0x8 },
-    { 0x2d,   BPF_REG_2,   BPF_REG_3, (int16_t)     54,        0x0 },
-    { 0xbf,   BPF_REG_5,   BPF_REG_4, (int16_t)      0,        0x0 },
-    {  0x7,   BPF_REG_5,   BPF_REG_0, (int16_t)      0,        0x9 },
-    { 0x2d,   BPF_REG_5,   BPF_REG_3, (int16_t)     51,        0x0 },
+    { 0xbf,   BPF_REG_6,   BPF_REG_2, (int16_t)      0,        0x0 },
+    {  0x7,   BPF_REG_6,   BPF_REG_0, (int16_t)      0,        0x8 },
+    { 0x2d,   BPF_REG_6,   BPF_REG_3, (int16_t)     58,        0x0 },
+    { 0xbf,   BPF_REG_4,   BPF_REG_2, (int16_t)      0,        0x0 },
+    {  0x7,   BPF_REG_4,   BPF_REG_0, (int16_t)      0,        0x9 },
+    { 0x2d,   BPF_REG_4,   BPF_REG_3, (int16_t)     55,        0x0 },
+    { 0xb7,   BPF_REG_4,   BPF_REG_0, (int16_t)      0,        0x8 },
     { 0xb7,   BPF_REG_5,   BPF_REG_0, (int16_t)      0,       0x14 },
     { 0xb7,   BPF_REG_0,   BPF_REG_0, (int16_t)      0,        0x9 },
-    { 0x71,   BPF_REG_6,   BPF_REG_2, (int16_t)      0,        0x0 },
+    { 0x71,   BPF_REG_6,   BPF_REG_6, (int16_t)      0,        0x0 },
     { 0x67,   BPF_REG_6,   BPF_REG_0, (int16_t)      0,       0x38 },
     { 0xc7,   BPF_REG_6,   BPF_REG_0, (int16_t)      0,       0x38 },
-    { 0x65,   BPF_REG_6,   BPF_REG_0, (int16_t)     10, 0xffffffff },
-    { 0xbf,   BPF_REG_2,   BPF_REG_4, (int16_t)      0,        0x0 },
-    {  0x7,   BPF_REG_2,   BPF_REG_0, (int16_t)      0,        0xd },
-    { 0x2d,   BPF_REG_2,   BPF_REG_3, (int16_t)     42,        0x0 },
-    { 0xbf,   BPF_REG_5,   BPF_REG_4, (int16_t)      0,        0x0 },
-    {  0x7,   BPF_REG_5,   BPF_REG_0, (int16_t)      0,        0xe },
-    { 0x2d,   BPF_REG_5,   BPF_REG_3, (int16_t)     39,        0x0 },
+    { 0x65,   BPF_REG_6,   BPF_REG_0, (int16_t)     11, 0xffffffff },
+    { 0xbf,   BPF_REG_5,   BPF_REG_2, (int16_t)      0,        0x0 },
+    {  0x7,   BPF_REG_5,   BPF_REG_0, (int16_t)      0,        0xd },
+    { 0x2d,   BPF_REG_5,   BPF_REG_3, (int16_t)     45,        0x0 },
+    { 0xbf,   BPF_REG_4,   BPF_REG_2, (int16_t)      0,        0x0 },
+    {  0x7,   BPF_REG_4,   BPF_REG_0, (int16_t)      0,        0xe },
+    { 0x2d,   BPF_REG_4,   BPF_REG_3, (int16_t)     42,        0x0 },
+    { 0xb7,   BPF_REG_4,   BPF_REG_0, (int16_t)      0,        0xd },
     { 0xb7,   BPF_REG_0,   BPF_REG_0, (int16_t)      0,        0xe },
-    { 0x71,   BPF_REG_5,   BPF_REG_2, (int16_t)      0,        0x0 },
+    { 0x71,   BPF_REG_5,   BPF_REG_5, (int16_t)      0,        0x0 },
     { 0xb7,   BPF_REG_6,   BPF_REG_0, (int16_t)      0,        0x8 },
-    { 0x2d,   BPF_REG_6,   BPF_REG_5, (int16_t)     35,        0x0 },
-    {  0xf,   BPF_REG_5,   BPF_REG_0, (int16_t)      0,        0x0 },
-    {  0xf,   BPF_REG_4,   BPF_REG_5, (int16_t)      0,        0x0 },
-    { 0x2d,   BPF_REG_4,   BPF_REG_3, (int16_t)     32,        0x0 },
+    { 0x2d,   BPF_REG_6,   BPF_REG_5, (int16_t)     37,        0x0 },
+    { 0xbf,   BPF_REG_6,   BPF_REG_2, (int16_t)      0,        0x0 },
+    {  0xf,   BPF_REG_6,   BPF_REG_0, (int16_t)      0,        0x0 },
+    {  0xf,   BPF_REG_6,   BPF_REG_5, (int16_t)      0,        0x0 },
+    { 0x2d,   BPF_REG_6,   BPF_REG_3, (int16_t)     33,        0x0 },
+    {  0xf,   BPF_REG_2,   BPF_REG_4, (int16_t)      0,        0x0 },
     { 0xbf,   BPF_REG_4,   BPF_REG_2, (int16_t)      0,        0x0 },
     {  0x7,   BPF_
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-acbcbdff7c` → 本草稿移入 `cases/defect/auto-nginx-acbcbdff7c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
