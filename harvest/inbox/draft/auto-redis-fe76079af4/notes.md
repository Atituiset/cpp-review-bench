# auto-redis-fe76079af4

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | redis/redis |
| 源 PR | [#15995](https://github.com/redis/redis/pull/15995) |
| 许可证 | RSALv2 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-21 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 29 |
| 编译错误数（gcc syntax-only） | 10（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #15995 (https://github.com/redis/redis/pull/15995)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 15995 RED-220725: Preserve BLESS across module value replacement :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -7734,10 +7734,13 @@ moduleType *RM_CreateDataType(RedisModuleCtx *ctx, const char *name, int encver,
  * writing or there is an active iterator, REDISMODULE_ERR is returned. */
 int RM_ModuleTypeSetValue(RedisModuleKey *key, moduleType *mt, void *value) {
     if (!(key->mode & REDISMODULE_WRITE) || key->iter) return REDISMODULE_ERR;
+    /* RM_DeleteKey clears BLESS state, so carry it across the replacement. */
+    int noevict = key->kv && blessIsNoEvict(key->kv);
     RM_DeleteKey(key);
     robj *o = createModuleObject(mt,value);
     setKey(key->ctx->client,key->db,key->key, &o,SETKEY_NO_SIGNAL);
     key->kv = o;
+    if (noevict) blessSetNoEvict(key->db, key->kv, 1);
     return REDISMODULE_OK;
 }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`createModuleObject`
- 外部函数：`dbDelete`
- 外部函数：`flag`
- 外部函数：`setKey`
- 外部函数：`signalKeyAsReady`
- 大写宏：`NULL`
- 大写宏：`OBJ_LIST`
- 大写宏：`OBJ_STREAM`
- 大写宏：`OBJ_ZSET`
- 大写宏：`REDISMODULE_ERR`
- 大写宏：`REDISMODULE_OK`
- 大写宏：`REDISMODULE_WRITE`
- 大写宏：`REDISMODULE_ZSET_RANGE_`
- 大写宏：`SETKEY_NO_SIGNAL`
- 外部类型：`Current`
- 外部类型：`End`
- 外部类型：`Fields`
- 外部类型：`Flag`
- 外部类型：`Iterator`
- 外部类型：`Key`
- 外部类型：`Lex`
- 外部类型：`List`
- 外部类型：`Opening`
- 外部类型：`RedisModuleCtx`
- 外部类型：`RedisModuleKey`
- 外部类型：`Score`
- 外部类型：`Start`
- 外部类型：`Stream`
- 外部类型：`Value`
- 外部类型：`Zset`
- 外部类型：`int64_t`
- 外部类型：`uint32_t`

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

1. 完成上面检查清单后评论 `/case accept auto-redis-fe76079af4` → 本草稿移入 `cases/defect/auto-redis-fe76079af4/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
