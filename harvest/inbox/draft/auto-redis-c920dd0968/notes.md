# auto-redis-c920dd0968

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | redis/redis |
| 源 PR | [#15433](https://github.com/redis/redis/pull/15433) |
| 许可证 | RSALv2 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-09-01 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 7 |
| 编译错误数（gcc syntax-only） | 4（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #15433 (https://github.com/redis/redis/pull/15433)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 733；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 15433 Fix signed overflow in BITFIELD #offset parsing :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -721,16 +721,22 @@ int getBitOffsetFromArgument(client *c, robj *o, uint64_t *offset, int hash, int
     /* Handle #<offset> form. */
     if (p[0] == '#' && hash && bits > 0) usehash = 1;
 
-    if (string2ll(p+usehash,plen-usehash,&loffset) == 0) {
+    if (string2ll(p+usehash,plen-usehash,&loffset) == 0 || loffset < 0) {
         addReplyError(c,err);
         return C_ERR;
     }
 
     /* Adjust the offset by 'bits' for #<offset> form. */
-    if (usehash) loffset *= bits;
+    if (usehash) {
+        if (loffset > LLONG_MAX / bits) {
+            addReplyError(c,err);
+            return C_ERR;
+        }
+        loffset *= bits;
+    }
 
     /* Limit offset to server.proto_max_bulk_len (512MB in bytes by default) */
-    if (loffset < 0 || (!mustObeyClient(c) && (loffset >> 3) >= server.proto_max_bulk_len))
+    if (!mustObeyClient(c) && (loffset >> 3) >= server.proto_max_bulk_len)
     {
         addReplyError(c,err);
         return C_ERR;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`addReplyError`
- 外部函数：`mustObeyClient`
- 外部函数：`string2ll`
- 大写宏：`C_ERR`
- 外部类型：`Adjust`
- 外部类型：`Handle`
- 外部类型：`Limit`

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

1. 完成上面检查清单后评论 `/case accept auto-redis-c920dd0968` → 本草稿移入 `cases/defect/auto-redis-c920dd0968/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
