# auto-redis-4ae2898dbc

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | redis/redis |
| 源 PR | [#15203](https://github.com/redis/redis/pull/15203) |
| 许可证 | RSALv2 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-09-01 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 35 |
| 编译错误数（gcc syntax-only） | 14（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #15203 (https://github.com/redis/redis/pull/15203)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 3066；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 15203 Security issues and bugs into unstable :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3058,13 +3058,13 @@ robj *rdbLoadObject(int rdbtype, rio *rdb, sds key, int dbid, int *error)
 
                         /* search for duplicate records */
                         sds field = sdstrynewlen(fstr, flen);
-                        int field_added = (field != NULL && dictAdd(dupSearchDict, field, NULL) == DICT_OK);
-                        if (!field_added || !lpSafeToAdd(lp, (size_t)flen + vlen)) {
+                        if (!field || !lpSafeToAdd(lp, (size_t)flen + vlen) ||
+                            dictAdd(dupSearchDict, field, NULL) != DICT_OK) {
                             rdbReportCorruptRDB("Hash zipmap with dup elements, or big length (%u)", flen);
                             /* If field was not added to dict, we still own it.
                              * If it was added, dict owns it and dictRelease will free it. */
-                            if (!field_added) sdsfree(field);
                             dictRelease(dupSearchDict);
+                            sdsfree(field);
                             lpFree(lp);
                             zfree(encoded);
                             o->ptr = NULL;
@@ -3584,7 +3584,6 @@ robj *rdbLoadObject(int rdbtype, rio *rdb, sds key, int dbid, int *error)
                         rdbReportCorruptRDB("Duplicated consumer PEL entry "
                                                 " loading a stream consumer "
                                                 "group");
-                        streamFreeNACK(s, nack);
                         decrRefCount(o);
                         return NULL;
                     }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`anetIsFifo`
- 外部函数：`check`
- 外部函数：`decrRefCount`
- 外部函数：`dictAdd`
- 外部函数：`dictRelease`
- 外部函数：`exit`
- 外部函数：`isRestoreContext`
- 外部函数：`length`
- 外部函数：`lpFree`
- 外部函数：`lpSafeToAdd`
- 外部函数：`proceed`
- 外部函数：`rdbCheckError`
- 外部函数：`rdbReportError`
- 外部函数：`read`
- 外部函数：`redis_check_rdb_main`
- 外部函数：`sdsfree`
- 外部函数：`sdstrynewlen`
- 外部函数：`serverLog`
- 外部函数：`snprintf`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 外部函数：`vsnprintf`
- 外部函数：`zfree`
- 大写宏：`CLIENT_ID_AOF`
- 大写宏：`DICT_OK`
- 大写宏：`FIFO`
- 大写宏：`LL_VERBOSE`
- 大写宏：`LL_WARNING`
- 大写宏：`NULL`
- 大写宏：`PEL`
- 大写宏：`RDB`
- 大写宏：`RESTORE`
- 大写宏：`VERBOSE`
- 外部类型：`Cannot`
- 外部类型：`Duplicated`
- 外部类型：`Failure`
- 外部类型：`Hash`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Internal`
- 外部类型：`Terminating`
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

1. 完成上面检查清单后评论 `/case accept auto-redis-4ae2898dbc` → 本草稿移入 `cases/defect/auto-redis-4ae2898dbc/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
