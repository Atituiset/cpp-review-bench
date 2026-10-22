# auto-redis-13b2b052f3

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | redis/redis |
| 源 PR | [#15891](https://github.com/redis/redis/pull/15891) |
| 许可证 | RSALv2 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-22 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 84 |
| 编译错误数（gcc syntax-only） | 37（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #15891 (https://github.com/redis/redis/pull/15891)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 15891 Bound a stuck crash report so the process still exits. :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -7716,6 +7716,7 @@ int redisFork(int purpose) {
          */
         server.in_fork_child = purpose;
         setupChildSignalHandlers();
+        crashWatchdogInit();
         setOOMScoreAdj(CONFIG_OOM_BGCHILD);
         updateDictResizePolicy();
         dismissMemoryInChild();
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`_exit`
- 外部函数：`adj`
- 外部函数：`atoi`
- 外部函数：`close`
- 外部函数：`dictSetResizeEnabled`
- 外部函数：`dismissMemoryInChild`
- 外部函数：`dup2`
- 外部函数：`exit`
- 外部函数：`fork`
- 外部函数：`getpid`
- 外部函数：`handler`
- 外部函数：`ll2string`
- 外部函数：`madvise`
- 外部函数：`open`
- 外部函数：`read`
- 外部函数：`serverAssert`
- 外部函数：`serverLog`
- 外部函数：`setsid`
- 外部函数：`sigaction`
- 外部函数：`sigemptyset`
- 外部函数：`snprintf`
- 外部函数：`strerror`
- 外部函数：`strlen`
- 外部函数：`time`
- 外部函数：`write`
- 外部函数：`zmadvise_dontneed`
- 大写宏：`CHILD_TYPE_MODULE`
- 大写宏：`CHILD_TYPE_NONE`
- 大写宏：`CONFIG_OOM_BGCHILD`
- 大写宏：`CONFIG_OOM_COUNT`
- 大写宏：`CONFIG_OOM_MASTER`
- 大写宏：`CONFIG_OOM_REPLICA`
- 大写宏：`COVERAGE_TEST`
- 大写宏：`C_ERR`
- 大写宏：`C_OK`
- 大写宏：`DICT_RESIZE_AVOID`
- 大写宏：`DICT_RESIZE_ENABLE`
- 大写宏：`DICT_RESIZE_FORBID`
- 大写宏：`HAVE_PROC_OOM_SCORE_ADJ`
- 大写宏：`LL_RAW`
- 大写宏：`LL_VERBOSE`
- 大写宏：`LL_WARNING`
- 大写宏：`MADV_DONTNEED`
- 大写宏：`NULL`
- 大写宏：`OOM_SCORE_ADJ_NO`
- 大写宏：`OOM_SCORE_RELATIVE`
- 大写宏：`O_APPEND`
- 大写宏：`O_CREAT`
- 大写宏：`O_RDONLY`
- 大写宏：`O_RDWR`
- 大写宏：`O_WRONLY`
- 大写宏：`SA_SIGINFO`
- 大写宏：`SERVER_CHILD_NOERROR_RETVAL`
- 大写宏：`SIGUSR1`
- 大写宏：`STDERR_FILENO`
- 大写宏：`STDIN_FILENO`
- 大写宏：`STDOUT_FILENO`
- 大写宏：`UNUSED`
- 外部类型：`And`
- 外部类型：`Backup`
- 外部类型：`Every`
- 外部类型：`If`
- 外部类型：`Otherwise`
- 外部类型：`Received`
- 外部类型：`Redis`
- 外部类型：`The`
- 外部类型：`Unable`
- 外部类型：`Unsupported`
- 外部类型：`We`
- 外部类型：`When`
- 外部类型：`sigaction`
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

1. 完成上面检查清单后评论 `/case accept auto-redis-13b2b052f3` → 本草稿移入 `cases/defect/auto-redis-13b2b052f3/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
