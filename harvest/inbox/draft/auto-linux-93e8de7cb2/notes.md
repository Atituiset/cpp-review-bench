# auto-linux-93e8de7cb2

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#a6656aefd328d544420ecec121fc898b7f067827](https://github.com/torvalds/linux/commit/a6656aefd328d544420ecec121fc898b7f067827) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-20 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 32 |
| 编译错误数（gcc syntax-only） | 10（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a6656aefd328d544420ecec121fc898b7f067827 (https://github.com/torvalds/linux/commit/a6656aefd328d544420ecec121fc898b7f067827)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 210；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a6656aefd328d544420ecec121fc898b7f067827 Merge tag 'vfs-7.4-rc1.shared.net-next.pidfd' of git://git.kernel.org/pub/scm/linux/kernel/git/vfs/vfs :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -149,6 +149,7 @@ EXPORT_SYMBOL(__scm_destroy);
 
 static inline int scm_replace_pid(struct scm_cookie *scm, struct pid *pid)
 {
+	struct pid *thread_pid;
 	int err;
 
 	/* drop all previous references */
@@ -158,7 +159,18 @@ static inline int scm_replace_pid(struct scm_cookie *scm, struct pid *pid)
 	if (unlikely(err))
 		return err;
 
-	scm->pid = pid;
+	/* A sender naming its own thread-group sends from the current thread. */
+	if (pid == task_tgid(current))
+		thread_pid = task_pid(current);
+	else
+		thread_pid = pid;
+
+	err = pidfs_register_pid(thread_pid);
+	if (unlikely(err))
+		return err;
+
+	scm->pid[PIDTYPE_TGID] = pid;
+	scm->pid[PIDTYPE_PID] = get_pid(thread_pid);
 	scm->creds.pid = pid_vnr(pid);
 	return 0;
 }
@@ -207,7 +219,8 @@ int __scm_send(struct socket *sock, struct msghdr *msg, struct scm_cookie *p)
 			if (err)
 				goto error;
 
-			if (!p->pid || pid_vnr(p->pid) != creds.pid) {
+			if (!p->pid[PIDTYPE_TGID] ||
+			    pid_vnr(p->pid[PIDTYPE_TGID]) != creds.pid) {
 				struct pid *pid;
 				err = -ESRCH;
 				pid = find_get_pid(creds.pid);
@@ -486,11 +499,16 @@ static bool scm_has_secdata(struct sock *sk)
 }
 #endif
 
-static void scm_pidfd_recv(struct msghdr *msg, struct scm_cookie *scm)
+static void scm_pidfd_recv(struct msghdr *msg, struct scm_cookie *scm,
+			   enum pid_type type, int cmsg_type)
 {
+	unsigned int flags = PIDFD_STALE;
 	struct file *pidfd_file = NULL;
+	struct pid *pid;
 	int len, pidfd;
 
+	pid = scm->pid[type];
+
 	/* put_cmsg() doesn't return an error if CMSG is truncated,
 	 * that's why we need to opencode these checks here.
 	 */
@@ -504,12 +522,15 @@ static void scm_pidfd_recv(struct msghdr *msg, struct scm_cookie *scm)
 		return;
 	}
 
-	if (!scm->pid)
+	if (!pid)
 		return;
 
-	pidfd = pidfd_prepare(scm->pid, PIDFD_STALE, &pidfd_file);
+	if (type == PIDTYPE_PID)
+		flags |= PIDFD_THREAD;
 
-	if (put_cmsg(msg, SOL_SOCKET, SCM_PIDFD, sizeof(int), &pidfd)) {
+	pidfd = pidfd_prepare(pid, flags, &pidfd_file);
+
+	if (put_cmsg(msg, SOL_SOCKET, cmsg_type, sizeof(int), &pidfd)) {
 		if (pidfd_file) {
 			put_unused_fd(pidfd);
 			fput(pidfd_file);
@@ -527,7 +548,7 @@ static bool __scm_recv_common(struct sock *sk, struct msghdr *msg,
 {
 	if (!msg->msg_control) {
 		if (sk->sk_scm_credentials || sk->sk_scm_pidfd ||
-		    scm->fp || scm_has_secdata(sk))
+		    sk->sk_scm_pidfd_thread || scm->fp || scm_has_secdata(sk))
 			msg->msg_flags |= MSG_CTRUNC;
 
 		scm_destroy(scm);
@@ -574,7 +595,10 @@ void scm_recv_unix(struct socket *sock, struct msghdr *msg,
 	}
 
 	if (sock->sk->sk_scm_pidfd)
-		scm_pidfd_recv(msg, scm);
+		scm_pidfd_recv(msg, scm, PIDTYPE_TGID, SCM_PIDFD);
+
+	if (sock->sk->sk_scm_pidfd_thread)
+		scm_pidfd_recv(msg, scm, PIDTYPE_PID, SCM_PIDFD_THREAD);
 
 	scm_destroy_cred(scm);
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`check_object_size`
- 外部函数：`find_get_pid`
- 外部函数：`fput`
- 外部函数：`memcpy`
- 外部函数：`min`
- 外部函数：`pid_vnr`
- 外部函数：`pidfd_prepare`
- 外部函数：`put_cmsg_compat`
- 外部函数：`put_unused_fd`
- 外部函数：`scm_destroy`
- 外部函数：`scm_destroy_cred`
- 外部函数：`unlikely`
- 外部函数：`unsafe_copy_to_user`
- 外部函数：`unsafe_put_user`
- 大写宏：`CMSG`
- 大写宏：`CMSG_DATA`
- 大写宏：`CMSG_LEN`
- 大写宏：`CMSG_SPACE`
- 大写宏：`CMSG_USER_DATA`
- 大写宏：`EFAULT`
- 大写宏：`ESRCH`
- 大写宏：`MSG_CMSG_COMPAT`
- 大写宏：`MSG_CTRUNC`
- 大写宏：`NULL`
- 大写宏：`PIDFD_STALE`
- 大写宏：`SCM_PIDFD`
- 大写宏：`SOL_SOCKET`
- 大写宏：`XXX`
- 外部类型：`cmsghdr`
- 外部类型：`file`
- 外部类型：`msghdr`
- 外部类型：`pid`
- 外部类型：`scm_cookie`
- 外部类型：`sock`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-93e8de7cb2` → 本草稿移入 `cases/defect/auto-linux-93e8de7cb2/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
