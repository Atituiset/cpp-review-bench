# auto-postgres-9b82a28fa6

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#d9b5a63f49d9d372a6609243eb0e9417a0a35e6f](https://github.com/postgres/postgres/commit/d9b5a63f49d9d372a6609243eb0e9417a0a35e6f) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-29 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 22 |
| 编译错误数（gcc syntax-only） | 5（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #d9b5a63f49d9d372a6609243eb0e9417a0a35e6f (https://github.com/postgres/postgres/commit/d9b5a63f49d9d372a6609243eb0e9417a0a35e6f)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 294；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT d9b5a63f49d9d372a6609243eb0e9417a0a35e6f Better express platform requirements in s_lock.h. :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -91,6 +91,7 @@ s_lock_stuck(const char *file, int line, const char *func)
 #endif
 }
 
+#ifdef USE_DEFAULT_S_LOCK
 /*
  * s_lock(lock) - platform-independent portion of waiting for a spinlock.
  */
@@ -110,6 +111,7 @@ s_lock(volatile slock_t *lock, const char *file, int line, const char *func)
 
 	return delayStatus.delays;
 }
+#endif
 
 #ifdef USE_DEFAULT_S_UNLOCK
 void
@@ -291,7 +293,7 @@ main()
 	printf("             if S_LOCK() and TAS() are working.\n");
 	fflush(stdout);
 
-	s_lock(&test_lock.lock, __FILE__, __LINE__, __func__);
+	S_LOCK(&test_lock.lock);
 
 	printf("S_LOCK_TEST: failed, lock not locked\n");
 	return 1;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`defined`
- 外部函数：`elog`
- 外部函数：`exit`
- 外部函数：`fflush`
- 外部函数：`fprintf`
- 外部函数：`init_spin_delay`
- 外部函数：`pg_prng_double`
- 外部函数：`pg_usleep`
- 外部函数：`pgstat_report_wait_end`
- 外部函数：`pgstat_report_wait_start`
- 外部函数：`printf`
- 大写宏：`CPU`
- 大写宏：`PANIC`
- 大写宏：`SPIN_DELAY`
- 大写宏：`S_LOCK`
- 大写宏：`S_LOCK_TEST`
- 大写宏：`TAS`
- 大写宏：`TAS_SPIN`
- 大写宏：`USE_DEFAULT_S_UNLOCK`
- 大写宏：`WAIT_EVENT_SPIN_DELAY`
- 外部类型：`Actively`
- 外部类型：`Block`
- 外部类型：`Once`
- 外部类型：`SpinDelayStatus`
- 外部类型：`We`
- 外部类型：`slock_t`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-9b82a28fa6` → 本草稿移入 `cases/defect/auto-postgres-9b82a28fa6/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
