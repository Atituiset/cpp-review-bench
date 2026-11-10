# auto-redis-4724526460

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | redis/redis |
| 源 PR | [#16165](https://github.com/redis/redis/pull/16165) |
| 许可证 | RSALv2 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-10 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 27 |
| 编译错误数（gcc syntax-only） | 38（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #16165 (https://github.com/redis/redis/pull/16165)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 5200；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 16165 Fix crashes when a module reads a zset emptied by ZPOP from its notification :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -4798,6 +4798,12 @@ void zrankGenericCommand(client *c, int reverse) {
     if ((zobj = lookupKeyReadOrReply(c, key, reply)) == NULL || checkType(c, zobj, OBJ_ZSET)) {
         return;
     }
+    /* A module may read a zset emptied by ZPOP from its notification,
+     * before the key is deleted. Reply as if the member is missing. */
+    if (zsetLength(zobj) == 0) {
+        addReply(c, reply);
+        return;
+    }
     if (server.memory_tracking_enabled)
         oldsize = kvobjAllocSize(zobj);
     serverAssertWithInfo(c, ele, sdsEncodedObject(ele));
@@ -5196,8 +5202,10 @@ void zrandmemberWithCountCommand(client *c, long l, int withscores) {
         uniq = 0;
     }
 
-    /* If count is zero, serve it ASAP to avoid special cases later. */
-    if (count == 0) {
+    /* If count is zero, serve it ASAP to avoid special cases later. The set
+     * can also be empty when a module reads it from the notification of a
+     * pop that removed its last member, before the key is deleted. */
+    if (count == 0 || size == 0) {
         addReply(c,shared.emptyarray);
         return;
     }
@@ -5436,6 +5444,12 @@ void zrandmemberCommand(client *c) {
         return;
     }
 
+    /* See zrandmemberWithCountCommand(). */
+    if (zsetLength(zset) == 0) {
+        addReplyNull(c);
+        return;
+    }
+
     if (server.memory_tracking_enabled)
         oldsize = kvobjAllocSize(zset);
     zsetTypeRandomElement(zset, zsetLength(zset), &ele,NULL);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`addReply`
- 外部函数：`checkType`
- 外部函数：`debugServerAssert`
- 外部函数：`dictGetFairRandomKey`
- 外部函数：`dictGetKey`
- 外部函数：`fast_float_strtod`
- 外部函数：`kvobjAllocSize`
- 外部函数：`lookupKeyReadOrReply`
- 外部函数：`lpLength`
- 外部函数：`lpRandomPair`
- 外部函数：`randomULong`
- 外部函数：`sdsEncodedObject`
- 外部函数：`sdslen`
- 外部函数：`serverAssert`
- 外部函数：`serverAssertWithInfo`
- 外部函数：`serverPanic`
- 外部函数：`zbtreeIteratorNext`
- 外部函数：`zbtreeIteratorSeekRank`
- 外部函数：`zbtreeLength`
- 大写宏：`ASAP`
- 大写宏：`NULL`
- 大写宏：`OBJ_ENCODING_BTREE`
- 大写宏：`OBJ_ENCODING_LISTPACK`
- 大写宏：`OBJ_ENCODING_SKIPLIST`
- 大写宏：`OBJ_ZSET`
- 大写宏：`UINT16_MAX`
- 外部类型：`If`
- 外部类型：`Unknown`
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

1. 完成上面检查清单后评论 `/case accept auto-redis-4724526460` → 本草稿移入 `cases/defect/auto-redis-4724526460/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
