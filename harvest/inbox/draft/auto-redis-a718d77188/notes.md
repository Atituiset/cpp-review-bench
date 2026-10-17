# auto-redis-a718d77188

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | redis/redis |
| 源 PR | [#15804](https://github.com/redis/redis/pull/15804) |
| 许可证 | RSALv2 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-17 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 67 |
| 编译错误数（gcc syntax-only） | 29（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #15804 (https://github.com/redis/redis/pull/15804)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 1944；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 15804 Redis 8.10.2 :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1940,9 +1940,18 @@ int ACLCheckAllUserCommandPerm(user *u, struct redisCommand *cmd, robj **argv, i
     return relevant_error;
 }
 
-/* High level API for checking if a client can execute the queued up command */
-int ACLCheckAllPerm(client *c, int *idxptr) {
-    return ACLCheckAllUserCommandPerm(c->user, c->cmd, c->argv, c->argc, getClientCachedKeyResult(c), idxptr);
+/* High level API for checking if a client can execute the queued up command.
+ *
+ * 'pcmd' is the pendingCommand that c->cmd / c->argv were populated from, and
+ * is used to reuse its cached key extraction result. It may be NULL for clients
+ * that don't build pendingCommand structs (module and script fake clients), in
+ * which case the keys are extracted from c->argv. */
+int ACLCheckAllPerm(client *c, pendingCommand *pcmd, int *idxptr) {
+    /* The cached key result holds key positions within pcmd->argv, so it is only
+     * usable for the command we are actually about to check. */
+    serverAssert(!pcmd || (pcmd->cmd == c->cmd));
+    return ACLCheckAllUserCommandPerm(c->user, c->cmd, c->argv, c->argc,
+                getClientCachedKeyResult(pcmd), idxptr);
 }
 
 /* If 'new' can access all channels 'original' could then return NULL;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`doesCommandHaveChannelsWithFlags`
- 外部函数：`doesCommandHaveKeys`
- 外部函数：`getChannelsFromCommand`
- 外部函数：`getClientCachedKeyResult`
- 外部函数：`getKeysFreeResult`
- 外部函数：`getKeysFromCommandWithSpecs`
- 外部函数：`likely`
- 外部函数：`listFirst`
- 外部函数：`listNext`
- 外部函数：`listNodeValue`
- 外部函数：`listRewind`
- 外部函数：`prefixmatch`
- 外部函数：`sdslen`
- 外部函数：`strcasecmp`
- 外部函数：`strcmp`
- 外部函数：`stringmatchlen`
- 大写宏：`ACL`
- 大写宏：`ACL_DENIED_CHANNEL`
- 大写宏：`ACL_DENIED_CMD`
- 大写宏：`ACL_DENIED_KEY`
- 大写宏：`ACL_OK`
- 大写宏：`ACL_READ_PERMISSION`
- 大写宏：`ACL_WRITE_PERMISSION`
- 大写宏：`ALLCHANNELS`
- 大写宏：`ALLKEYS`
- 大写宏：`API`
- 大写宏：`CMD_CHANNEL_PATTERN`
- 大写宏：`CMD_CHANNEL_PUBLISH`
- 大写宏：`CMD_CHANNEL_SUBSCRIBE`
- 大写宏：`CMD_KEY_ACCESS`
- 大写宏：`CMD_KEY_DELETE`
- 大写宏：`CMD_KEY_INSERT`
- 大写宏：`CMD_KEY_PREFIX`
- 大写宏：`CMD_KEY_UPDATE`
- 大写宏：`CMD_NO_AUTH`
- 大写宏：`C_ERR`
- 大写宏：`C_OK`
- 大写宏：`GETKEYS_RESULT_INIT`
- 大写宏：`GET_KEYSPEC_DEFAULT`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 大写宏：`PUBLISH`
- 大写宏：`SDS`
- 大写宏：`SELECTOR_FLAG_`
- 大写宏：`SELECTOR_FLAG_ALLCHANNELS`
- 大写宏：`SELECTOR_FLAG_ALLCOMMANDS`
- 大写宏：`SELECTOR_FLAG_ALLKEYS`
- 大写宏：`UNSUBSCRIBE`
- 大写宏：`USER_COMMAND_BITS_COUNT`
- 外部类型：`Channel`
- 外部类型：`Check`
- 外部类型：`First`
- 外部类型：`For`
- 外部类型：`High`
- 外部类型：`If`
- 外部类型：`Is`
- 外部类型：`Pub`
- 外部类型：`Quick`
- 外部类型：`Redis`
- 外部类型：`Regular`
- 外部类型：`Return`
- 外部类型：`See`
- 外部类型：`Stop`
- 外部类型：`Sub`
- 外部类型：`Test`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`We`
- 外部类型：`When`
- 外部类型：`redisCommand`
- 外部类型：`size_t`
- 外部类型：`uint32_t`
- 外部类型：`uint64_t`

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

1. 完成上面检查清单后评论 `/case accept auto-redis-a718d77188` → 本草稿移入 `cases/defect/auto-redis-a718d77188/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
