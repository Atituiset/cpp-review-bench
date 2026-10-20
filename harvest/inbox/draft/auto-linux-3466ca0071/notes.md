# auto-linux-3466ca0071

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
| 外部依赖数（dep_count） | 36 |
| 编译错误数（gcc syntax-only） | 16（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a6656aefd328d544420ecec121fc898b7f067827 (https://github.com/torvalds/linux/commit/a6656aefd328d544420ecec121fc898b7f067827)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1932；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a6656aefd328d544420ecec121fc898b7f067827 Merge tag 'vfs-7.4-rc1.shared.net-next.pidfd' of git://git.kernel.org/pub/scm/linux/kernel/git/vfs/vfs :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1578,6 +1578,13 @@ int sk_setsockopt(struct sock *sk, int level, int optname,
 			ret = -EOPNOTSUPP;
 		break;
 
+	case SO_PASSPIDFD_THREAD:
+		if (sk_is_unix(sk))
+			sk->sk_scm_pidfd_thread = valbool;
+		else
+			ret = -EOPNOTSUPP;
+		break;
+
 	case SO_PASSRIGHTS:
 		if (sk_is_unix(sk))
 			sk->sk_scm_rights = valbool;
@@ -1729,6 +1736,50 @@ static int groups_to_user(sockptr_t dst, const struct group_info *src)
 	return 0;
 }
 
+/* Hand out a pidfd for @type of the socket's peer via SO_PEERPIDFD*. */
+static int sk_getsockopt_peerpidfd(struct sock *sk, enum pid_type type,
+				   sockptr_t optval, sockptr_t optlen, int len)
+{
+	struct file *pidfd_file = NULL;
+	unsigned int flags = 0;
+	struct pid *peer_pid;
+	int pidfd;
+
+	if (len > sizeof(pidfd))
+		len = sizeof(pidfd);
+
+	spin_lock(&sk->sk_peer_lock);
+	peer_pid = get_pid(sk->sk_peer_pid[type]);
+	spin_unlock(&sk->sk_peer_lock);
+
+	if (!peer_pid)
+		return -ENODATA;
+
+	/* The use of PIDFD_STALE requires stashing of struct pid on pidfs
+	 * with pidfs_register_pid() and only AF_UNIX is prepared for this.
+	 */
+	if (sk_is_unix(sk))
+		flags |= PIDFD_STALE;
+	if (type == PIDTYPE_PID)
+		flags |= PIDFD_THREAD;
+
+	pidfd = pidfd_prepare(peer_pid, flags, &pidfd_file);
+	put_pid(peer_pid);
+	if (pidfd < 0)
+		return pidfd;
+
+	if (copy_to_sockptr(optval, &pidfd, len) ||
+	    copy_to_sockptr(optlen, &len, sizeof(int))) {
+		put_unused_fd(pidfd);
+		fput(pidfd_file);
+
+		return -EFAULT;
+	}
+
+	fd_install(pidfd, pidfd_file);
+	return 0;
+}
+
 int sk_getsockopt(struct sock *sk, int level, int optname,
 		  sockptr_t optval, sockptr_t optlen)
 {
@@ -1893,6 +1944,13 @@ int sk_getsockopt(struct sock *sk, int level, int optname,
 		v.val = sk->sk_scm_pidfd;
 		break;
 
+	case SO_PASSPIDFD_THREAD:
+		if (!sk_is_unix(sk))
+			return -EOPNOTSUPP;
+
+		v.val = sk->sk_scm_pidfd_thread;
+		break;
+
 	case SO_PASSRIGHTS:
 		if (!sk_is_unix(sk))
 			return -EOPNOTSUPP;
@@ -1907,7 +1965,8 @@ int sk_getsockopt(struct sock *sk, int level, int optname,
 			len = sizeof(peercred);
 
 		spin_lock(&sk->sk_peer_lock);
-		cred_to_ucred(sk->sk_peer_pid, sk->sk_peer_cred, &peercred);
+		cred_to_ucred(sk->sk_peer_pid[PIDTYPE_TGID], sk->sk_peer_cred,
+			      &peercred);
 		spin_unlock(&sk->sk_peer_lock);
 
 		if (copy_to_sockptr(optval, &peercred, len))
@@ -1916,45 +1975,14 @@ int sk_getsockopt(struct sock *sk, int level, int optname,
 	}
 
 	case SO_PEERPIDFD:
-	{
-		struct pid *peer_pid;
-		struct file *pidfd_file = NULL;
-		unsigned int flags = 0;
-		int pidfd;
-
-		if (len > sizeof(pidfd))
-			len = sizeof(pidfd);
-
-		spin_lock(&sk->sk_peer_lock);
-		peer_pid = get_pid(sk->sk_peer_pid);
-		spin_unlock(&sk->sk_peer_lock);
-
-		if (!peer_pid)
-			return -ENODATA;
-
-		/* The use of PIDFD_STALE requires stashing of struct pid
-		 * on pidfs with pidfs_register_pid() and only AF_UNIX
-		 * were prepared for this.
-		 */
-		if (sk->sk_family == AF_UNIX)
-			flags = PIDFD_STALE;
+		return sk_getsockopt_peerpidfd(sk, PIDTYPE_TGID, optval, optlen, len);
 
-		pidfd = pidfd_prepare(peer_pid, flags, &pidfd_file);
-		put_pid(peer_pid);
-		if (pidfd < 0)
-			return pidfd;
-
-		if (copy_to_sockptr(optval, &pidfd, len) ||
-		    copy_to_sockptr(optlen, &len, sizeof(int))) {
-			put_unused_fd(pidfd);
-			fput(pidfd_file);
-
-			return -EFAULT;
-		}
+	case SO_PEERPIDFD_THREAD:
+		/* Only AF_UNIX records the peer's connecting thread. */
+		if (!sk_is_unix(sk))
+			return -EOPNOTSUPP;
 
-		fd_install(pidfd, pidfd_file);
-		return 0;
-	}
+		return sk_getsockopt_peerpidfd(sk, PIDTYPE_PID, optval, optlen, len);
 
 	case SO_PEERGROUPS:
 	{
@@ -2380,7 +2408,7 @@ static void __sk_destruct(struct rcu_head *head)
 
 	/* We do not need to acquire sk->sk_peer_lock, we are the last user. */
 	put_cred(sk->sk_peer_cred);
-	put_pid(sk->sk_peer_pid);
+	put_pids(sk->sk_peer_pid);
 
 	if (likely(sk->sk_net_refcnt)) {
 		put_net_track(net, &sk->ns_tracker);
@@ -3785,7 +3813,7 @@ void sock_init_data_uid(struct socket *sock
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`copy_to_sockptr`
- 外部函数：`current_user_ns`
- 外部函数：`fd_install`
- 外部函数：`fput`
- 外部函数：`from_kgid_munged`
- 外部函数：`from_kuid_munged`
- 外部函数：`get_pid`
- 外部函数：`likely`
- 外部函数：`pid_vnr`
- 外部函数：`pidfd_prepare`
- 外部函数：`pidfs_register_pid`
- 外部函数：`put_cred`
- 外部函数：`put_net_track`
- 外部函数：`put_pid`
- 外部函数：`put_unused_fd`
- 外部函数：`sk_is_unix`
- 外部函数：`spin_lock`
- 外部函数：`spin_lock_init`
- 外部函数：`spin_unlock`
- 大写宏：`AF_UNIX`
- 大写宏：`EFAULT`
- 大写宏：`ENODATA`
- 大写宏：`EOPNOTSUPP`
- 大写宏：`NULL`
- 大写宏：`PIDFD_STALE`
- 大写宏：`SO_PASSRIGHTS`
- 大写宏：`SO_PEERGROUPS`
- 大写宏：`SO_PEERPIDFD`
- 外部类型：`The`
- 外部类型：`We`
- 外部类型：`cred`
- 外部类型：`file`
- 外部类型：`pid`
- 外部类型：`sock`
- 外部类型：`sockptr_t`
- 外部类型：`ucred`
- 外部类型：`user_namespace`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-3466ca0071` → 本草稿移入 `cases/defect/auto-linux-3466ca0071/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
