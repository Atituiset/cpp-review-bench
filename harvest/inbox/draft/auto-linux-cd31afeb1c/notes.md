# auto-linux-cd31afeb1c

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
| 外部依赖数（dep_count） | 31 |
| 编译错误数（gcc syntax-only） | 21（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a6656aefd328d544420ecec121fc898b7f067827 (https://github.com/torvalds/linux/commit/a6656aefd328d544420ecec121fc898b7f067827)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 751；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a6656aefd328d544420ecec121fc898b7f067827 Merge tag 'vfs-7.4-rc1.shared.net-next.pidfd' of git://git.kernel.org/pub/scm/linux/kernel/git/vfs/vfs :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -737,57 +737,53 @@ static void unix_release_sock(struct sock *sk, int embrion)
 }
 
 struct unix_peercred {
-	struct pid *peer_pid;
+	DECLARE_PIDS(peer_pid, PIDTYPE_TGID);
 	const struct cred *peer_cred;
 };
 
 static inline int prepare_peercred(struct unix_peercred *peercred)
 {
-	struct pid *pid;
 	int err;
 
-	pid = task_tgid(current);
-	err = pidfs_register_pid(pid);
-	if (likely(!err)) {
-		peercred->peer_pid = get_pid(pid);
-		peercred->peer_cred = get_current_cred();
+	get_task_pids(peercred->peer_pid, current);
+	err = pidfs_register_pids(peercred->peer_pid);
+	if (unlikely(err)) {
+		put_pids(peercred->peer_pid);
+		return err;
 	}
-	return err;
+
+	peercred->peer_cred = get_current_cred();
+	return 0;
 }
 
 static void drop_peercred(struct unix_peercred *peercred)
 {
 	const struct cred *cred = NULL;
-	struct pid *pid = NULL;
 
 	might_sleep();
 
-	swap(peercred->peer_pid, pid);
+	put_pids(peercred->peer_pid);
 	swap(peercred->peer_cred, cred);
-
-	put_pid(pid);
 	put_cred(cred);
 }
 
 static inline void init_peercred(struct sock *sk,
 				 const struct unix_peercred *peercred)
 {
-	sk->sk_peer_pid = peercred->peer_pid;
+	memcpy(sk->sk_peer_pid, peercred->peer_pid, sizeof(sk->sk_peer_pid));
 	sk->sk_peer_cred = peercred->peer_cred;
 }
 
 static void update_peercred(struct sock *sk, struct unix_peercred *peercred)
 {
 	const struct cred *old_cred;
-	struct pid *old_pid;
 
 	spin_lock(&sk->sk_peer_lock);
-	old_pid = sk->sk_peer_pid;
+	swap_pids(sk->sk_peer_pid, peercred->peer_pid);
 	old_cred = sk->sk_peer_cred;
-	init_peercred(sk, peercred);
+	sk->sk_peer_cred = peercred->peer_cred;
 	spin_unlock(&sk->sk_peer_lock);
 
-	peercred->peer_pid = old_pid;
 	peercred->peer_cred = old_cred;
 }
 
@@ -796,14 +792,15 @@ static void copy_peercred(struct sock *sk, struct sock *peersk)
 	lockdep_assert_held(&unix_sk(peersk)->lock);
 
 	spin_lock(&sk->sk_peer_lock);
-	sk->sk_peer_pid = get_pid(peersk->sk_peer_pid);
+	get_pids(sk->sk_peer_pid, peersk->sk_peer_pid);
 	sk->sk_peer_cred = get_cred(peersk->sk_peer_cred);
 	spin_unlock(&sk->sk_peer_lock);
 }
 
 static bool unix_may_passcred(const struct sock *sk)
 {
-	return sk->sk_scm_credentials || sk->sk_scm_pidfd;
+	return sk->sk_scm_credentials || sk->sk_scm_pidfd ||
+		sk->sk_scm_pidfd_thread;
 }
 
 static int unix_listen(struct socket *sock, int backlog)
@@ -1060,6 +1057,7 @@ static bool unix_bpf_bypass_getsockopt(int level, int optname)
 	if (level == SOL_SOCKET) {
 		switch (optname) {
 		case SO_PEERPIDFD:
+		case SO_PEERPIDFD_THREAD:
 			return true;
 		default:
 			return false;
@@ -1973,7 +1971,7 @@ static void unix_destruct_scm(struct sk_buff *skb)
 {
 	struct scm_cookie scm = {};
 
-	swap(scm.pid, UNIXCB(skb).pid);
+	swap_pids(scm.pid, UNIXCB(skb).pid);
 
 	if (UNIXCB(skb).fp)
 		unix_detach_fds(&scm, skb);
@@ -1991,7 +1989,7 @@ static int unix_scm_to_skb(struct scm_cookie *scm, struct sk_buff *skb, bool sen
 {
 	int err = 0;
 
-	UNIXCB(skb).pid = get_pid(scm->pid);
+	get_pids(UNIXCB(skb).pid, scm->pid);
 	UNIXCB(skb).uid = scm->creds.uid;
 	UNIXCB(skb).gid = scm->creds.gid;
 	UNIXCB(skb).fp = NULL;
@@ -2005,7 +2003,8 @@ static int unix_scm_to_skb(struct scm_cookie *scm, struct sk_buff *skb, bool sen
 
 static void unix_skb_to_scm(struct sk_buff *skb, struct scm_cookie *scm)
 {
-	scm_set_cred(scm, UNIXCB(skb).pid, UNIXCB(skb).uid, UNIXCB(skb).gid);
+	get_pids(scm->pid, UNIXCB(skb).pid);
+	scm_set_cred(scm, UNIXCB(skb).pid[PIDTYPE_TGID], UNIXCB(skb).uid, UNIXCB(skb).gid);
 	unix_set_secdata(scm, skb);
 }
 
@@ -2025,30 +2024,35 @@ static void unix_skb_to_scm(struct sk_buff *skb, struct scm_cookie *scm)
 static int unix_maybe_add_creds(struct sk_buff *skb, const struct sock *sk,
 				const struct sock *other)
 {
-	if (UNIXCB(skb).pid)
+	if (UNIXCB(skb).pid[PIDTYPE_TGID])
 		return 0;
 
 	if (unix_may_passcred(sk) || unix_may_passcred(other) ||
 	    !other->sk_socket) {
-		struct pid *pid;
 		int err;
 
-		pid = task_tgid(current);
-		err = pidfs_register_pid(pid);
-		if (u
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`current_uid_gid`
- 外部函数：`get_cred`
- 外部函数：`get_current_cred`
- 外部函数：`get_pid`
- 外部函数：`gid_eq`
- 外部函数：`likely`
- 外部函数：`lockdep_assert_held`
- 外部函数：`might_sleep`
- 外部函数：`pidfs_register_pid`
- 外部函数：`put_cred`
- 外部函数：`put_pid`
- 外部函数：`scm_set_cred`
- 外部函数：`spin_lock`
- 外部函数：`spin_unlock`
- 外部函数：`swap`
- 外部函数：`task_tgid`
- 外部函数：`uid_eq`
- 外部函数：`unix_destroy_fpl`
- 外部函数：`unix_sk`
- 外部函数：`unlikely`
- 大写宏：`NULL`
- 大写宏：`SOL_SOCKET`
- 大写宏：`SO_PEERPIDFD`
- 大写宏：`UNIXCB`
- 外部类型：`Copy`
- 外部类型：`Never`
- 外部类型：`cred`
- 外部类型：`pid`
- 外部类型：`scm_cookie`
- 外部类型：`sk_buff`
- 外部类型：`sock`
- 外部类型：`socket`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-cd31afeb1c` → 本草稿移入 `cases/defect/auto-linux-cd31afeb1c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
