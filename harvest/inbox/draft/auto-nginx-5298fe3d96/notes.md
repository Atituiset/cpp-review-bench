# auto-nginx-5298fe3d96

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | nginx/nginx |
| 源 PR | [#1680](https://github.com/nginx/nginx/pull/1680) |
| 许可证 | BSD-2-Clause |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 2 |
| 编译错误数（gcc syntax-only） | 6（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1680 (https://github.com/nginx/nginx/pull/1680)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 2213；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 1680 Resolver: avoid signed overflow assembling values from DNS responses :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2210,7 +2210,7 @@ ngx_resolver_process_a(ngx_resolver_t *r, u_char *buf, size_t n,
         type = (an->type_hi << 8) + an->type_lo;
         class = (an->class_hi << 8) + an->class_lo;
         len = (an->len_hi << 8) + an->len_lo;
-        ttl = (an->ttl[0] << 24) + (an->ttl[1] << 16)
+        ttl = ((uint32_t) an->ttl[0] << 24) + (an->ttl[1] << 16)
             + (an->ttl[2] << 8) + (an->ttl[3]);
 
         if (class != 1) {
@@ -2372,7 +2372,7 @@ ngx_resolver_process_a(ngx_resolver_t *r, u_char *buf, size_t n,
 
             if (type == NGX_RESOLVE_A) {
 
-                addr[j] = htonl((buf[i] << 24) + (buf[i + 1] << 16)
+                addr[j] = htonl(((uint32_t) buf[i] << 24) + (buf[i + 1] << 16)
                                 + (buf[i + 2] << 8) + (buf[i + 3]));
 
                 if (++j == naddrs) {
@@ -2767,7 +2767,7 @@ ngx_resolver_process_srv(ngx_resolver_t *r, u_char *buf, size_t n,
         type = (an->type_hi << 8) + an->type_lo;
         class = (an->class_hi << 8) + an->class_lo;
         len = (an->len_hi << 8) + an->len_lo;
-        ttl = (an->ttl[0] << 24) + (an->ttl[1] << 16)
+        ttl = ((uint32_t) an->ttl[0] << 24) + (an->ttl[1] << 16)
             + (an->ttl[2] << 8) + (an->ttl[3]);
 
         if (class != 1) {
@@ -3352,7 +3352,7 @@ ngx_resolver_process_ptr(ngx_resolver_t *r, u_char *buf, size_t n,
         type = (an->type_hi << 8) + an->type_lo;
         class = (an->class_hi << 8) + an->class_lo;
         len = (an->len_hi << 8) + an->len_lo;
-        ttl = (an->ttl[0] << 24) + (an->ttl[1] << 16)
+        ttl = ((uint32_t) an->ttl[0] << 24) + (an->ttl[1] << 16)
             + (an->ttl[2] << 8) + (an->ttl[3]);
 
         if (class != 1) {
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`htonl`
- 大写宏：`NGX_RESOLVE_A`

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

1. 完成上面检查清单后评论 `/case accept auto-nginx-5298fe3d96` → 本草稿移入 `cases/defect/auto-nginx-5298fe3d96/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
