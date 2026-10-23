# auto-linux-5e951032bd

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#79132ddf81d78d9b84d9b8f05c7c5c3a3e912537](https://github.com/torvalds/linux/commit/79132ddf81d78d9b84d9b8f05c7c5c3a3e912537) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-23 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 47 |
| 编译错误数（gcc syntax-only） | 23（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #79132ddf81d78d9b84d9b8f05c7c5c3a3e912537 (https://github.com/torvalds/linux/commit/79132ddf81d78d9b84d9b8f05c7c5c3a3e912537)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1211；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 79132ddf81d78d9b84d9b8f05c7c5c3a3e912537 Merge tag 'seccomp-v7.4-rc1' of git://git.kernel.org/pub/scm/linux/kernel/git/kees/linux :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -205,6 +205,7 @@ static inline void seccomp_cache_prepare(struct seccomp_filter *sfilter)
  * @log: true if all actions except for SECCOMP_RET_ALLOW should be logged
  * @wait_killable_recv: Put notifying process in killable state once the
  *			notification is received by the userspace listener.
+ * @restart_before_recv: Restart interrupted syscalls before notification receipt.
  * @prev: points to a previously installed, or inherited, filter
  * @prog: the BPF program to evaluate
  * @notif: the struct that holds all notification related information
@@ -226,6 +227,7 @@ struct seccomp_filter {
 	refcount_t users;
 	bool log;
 	bool wait_killable_recv;
+	bool restart_before_recv;
 	struct action_cache cache;
 	struct seccomp_filter *prev;
 	struct bpf_prog *prog;
@@ -953,6 +955,8 @@ static long seccomp_attach_filter(unsigned int flags,
 	/* Set wait killable flag, if present. */
 	if (flags & SECCOMP_FILTER_FLAG_WAIT_KILLABLE_RECV)
 		filter->wait_killable_recv = true;
+	if (flags & SECCOMP_FILTER_FLAG_RESTART_BEFORE_RECV)
+		filter->restart_before_recv = true;
 
 	/*
 	 * If there is an existing filter, make it the prev and don't drop its
@@ -1208,8 +1212,12 @@ static int seccomp_do_user_notification(int this_syscall,
 			 * Check to see whether we should switch to wait
 			 * killable. Only return the interrupted error if not.
 			 */
-			if (!(!wait_killable && should_sleep_killable(match, &n)))
+			if (!(!wait_killable && should_sleep_killable(match, &n))) {
+				if (err == -ERESTARTSYS && match->restart_before_recv &&
+				    n.state == SECCOMP_NOTIFY_INIT)
+					err = -ERESTARTNOINTR;
 				goto interrupted;
+			}
 		}
 
 		addfd = list_first_entry_or_null(&n.addfd,
@@ -1735,7 +1743,8 @@ static long seccomp_notify_addfd(struct seccomp_filter *filter,
 	if (addfd.newfd && !(addfd.flags & SECCOMP_ADDFD_FLAG_SETFD))
 		return -EINVAL;
 
-	kaddfd.file = fget(addfd.srcfd);
+	/* Allow O_PATH files, as SCM_RIGHTS and pidfd_getfd() do. */
+	kaddfd.file = fget_raw(addfd.srcfd);
 	if (!kaddfd.file)
 		return -EBADF;
 
@@ -1808,10 +1817,13 @@ static long seccomp_notify_addfd(struct seccomp_filter *filter,
 	 * We need to check again if the addfd request has been handled,
 	 * and if not, we will remove it from the queue.
 	 */
-	if (list_empty(&kaddfd.list))
+	if (list_empty(&kaddfd.list)) {
 		ret = kaddfd.ret;
-	else
+	} else {
 		list_del(&kaddfd.list);
+		if (addfd.flags & SECCOMP_ADDFD_FLAG_SEND)
+			knotif->state = SECCOMP_NOTIFY_SENT;
+	}
 
 out_unlock:
 	mutex_unlock(&filter->notify_lock);
@@ -1977,10 +1989,10 @@ static long seccomp_set_mode_filter(unsigned int flags,
 		return -EINVAL;
 
 	/*
-	 * The SECCOMP_FILTER_FLAG_WAIT_KILLABLE_SENT flag doesn't make sense
-	 * without the SECCOMP_FILTER_FLAG_NEW_LISTENER flag.
+	 * Notification wait flags require a userspace listener.
 	 */
-	if ((flags & SECCOMP_FILTER_FLAG_WAIT_KILLABLE_RECV) &&
+	if ((flags & (SECCOMP_FILTER_FLAG_WAIT_KILLABLE_RECV |
+		      SECCOMP_FILTER_FLAG_RESTART_BEFORE_RECV)) &&
 	    ((flags & SECCOMP_FILTER_FLAG_NEW_LISTENER) == 0))
 		return -EINVAL;
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`fget`
- 外部函数：`list_del`
- 外部函数：`list_empty`
- 外部函数：`list_first_entry_or_null`
- 外部函数：`mutex_unlock`
- 大写宏：`BPF`
- 大写宏：`DECLARE_BITMAP`
- 大写宏：`EBADF`
- 大写宏：`EINVAL`
- 大写宏：`INIT`
- 大写宏：`REPLIED`
- 大写宏：`SECCOMP_ADDFD_FLAG_SETFD`
- 大写宏：`SECCOMP_ARCH_COMPAT`
- 大写宏：`SECCOMP_ARCH_COMPAT_NR`
- 大写宏：`SECCOMP_ARCH_NATIVE_NR`
- 大写宏：`SECCOMP_FILTER_FLAG_NEW_LISTENER`
- 大写宏：`SECCOMP_FILTER_FLAG_WAIT_KILLABLE_RECV`
- 大写宏：`SECCOMP_FILTER_FLAG_WAIT_KILLABLE_SENT`
- 大写宏：`SECCOMP_NOTIFY_INIT`
- 大写宏：`SECCOMP_NOTIFY_REPLIED`
- 大写宏：`SECCOMP_NOTIFY_SENT`
- 大写宏：`SECCOMP_RET_ALLOW`
- 大写宏：`SECCOMP_RET_USER_NOTIF`
- 大写宏：`SENT`
- 外部类型：`Check`
- 外部类型：`If`
- 外部类型：`Notification`
- 外部类型：`Once`
- 外部类型：`Only`
- 外部类型：`Put`
- 外部类型：`Set`
- 外部类型：`Signals`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`We`
- 外部类型：`When`
- 外部类型：`atomic_t`
- 外部类型：`bpf_prog`
- 外部类型：`completion`
- 外部类型：`list_head`
- 外部类型：`mutex`
- 外部类型：`pid`
- 外部类型：`refcount_t`
- 外部类型：`seccomp_data`
- 外部类型：`task_struct`
- 外部类型：`that`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-5e951032bd` → 本草稿移入 `cases/defect/auto-linux-5e951032bd/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
