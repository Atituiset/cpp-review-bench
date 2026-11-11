# auto-linux-b9a7933142

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#5f5be6584cf5af0d70ee2fd40aa6264da18617ec](https://github.com/torvalds/linux/commit/5f5be6584cf5af0d70ee2fd40aa6264da18617ec) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-11 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 45 |
| 编译错误数（gcc syntax-only） | 43（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #5f5be6584cf5af0d70ee2fd40aa6264da18617ec (https://github.com/torvalds/linux/commit/5f5be6584cf5af0d70ee2fd40aa6264da18617ec)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 964；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 5f5be6584cf5af0d70ee2fd40aa6264da18617ec Merge tag 'vfs-7.4-rc3.fixes' of git://git.kernel.org/pub/scm/linux/kernel/git/vfs/vfs :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -961,7 +961,7 @@ void exit_aio(struct mm_struct *mm)
 	if (!table)
 		return;
 
-	atomic_set(&wait.count, table->nr);
+	atomic_set(&wait.count, table->nr + 1);
 	init_completion(&wait.comp);
 
 	skipped = 0;
@@ -985,7 +985,7 @@ void exit_aio(struct mm_struct *mm)
 		kill_ioctx(mm, ctx, &wait);
 	}
 
-	if (!atomic_sub_and_test(skipped, &wait.count)) {
+	if (!atomic_sub_and_test(skipped + 1, &wait.count)) {
 		/* Wait until all IO for the context are done. */
 		wait_for_completion(&wait.comp);
 	}
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__counted_by`
- 外部函数：`aio_setup_ring`
- 外部函数：`atomic_set`
- 外部函数：`atomic_sub_and_test`
- 外部函数：`atomic_xchg`
- 外部函数：`decremented`
- 外部函数：`free_ioctx`
- 外部函数：`free_ioctx_reqs`
- 外部函数：`init_completion`
- 外部函数：`io_setup`
- 外部函数：`larger`
- 外部函数：`percpu_ref_kill`
- 外部函数：`rcu_access_pointer`
- 外部函数：`rcu_dereference_raw`
- 外部函数：`spin_lock`
- 外部函数：`spin_unlock`
- 外部函数：`use`
- 外部函数：`vm_munmap`
- 外部函数：`wait_for_completion`
- 外部函数：`wake_up_all`
- 大写宏：`EAGAIN`
- 大写宏：`EINVAL`
- 大写宏：`NULL`
- 大写宏：`RCU`
- 大写宏：`RCU_INIT_POINTER`
- 大写宏：`WARN_ON`
- 外部类型：`For`
- 外部类型：`It`
- 外部类型：`Size`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Wait`
- 外部类型：`We`
- 外部类型：`atomic_t`
- 外部类型：`completion`
- 外部类型：`file`
- 外部类型：`folio`
- 外部类型：`io_event`
- 外部类型：`list_head`
- 外部类型：`mm_struct`
- 外部类型：`mutex`
- 外部类型：`percpu_ref`
- 外部类型：`rcu_head`
- 外部类型：`rcu_work`
- 外部类型：`spinlock_t`
- 外部类型：`wait_queue_head_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-b9a7933142` → 本草稿移入 `cases/defect/auto-linux-b9a7933142/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
