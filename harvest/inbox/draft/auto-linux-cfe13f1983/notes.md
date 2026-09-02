# auto-linux-cfe13f1983

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#9a58da80053f992b285b6b7bebc694b0f284c443](https://github.com/torvalds/linux/commit/9a58da80053f992b285b6b7bebc694b0f284c443) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-09-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 15 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #9a58da80053f992b285b6b7bebc694b0f284c443 (https://github.com/torvalds/linux/commit/9a58da80053f992b285b6b7bebc694b0f284c443)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 387；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 9a58da80053f992b285b6b7bebc694b0f284c443 Merge tag 'ksmbd-for-7.3-rc2' of git://git.kernel.org/pub/scm/linux/kernel/git/linkinjeon/smb :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -13,6 +13,7 @@
 #include "mgmt/ksmbd_ida.h"
 #include "mgmt/user_session.h"
 #include "connection.h"
+#include "vfs_cache.h"
 #include "compress.h"
 #include "transport_tcp.h"
 #include "transport_rdma.h"
@@ -384,12 +385,12 @@ static void ksmbd_conn_cancel_async_requests(struct ksmbd_conn *conn)
 	spin_lock(&conn->request_lock);
 	list_for_each_entry_safe(work, tmp, &conn->async_requests,
 				 async_request_entry) {
-		if (work->state != KSMBD_WORK_ACTIVE)
+		if (cmpxchg(&work->state, KSMBD_WORK_ACTIVE,
+			    KSMBD_WORK_CANCELLED) != KSMBD_WORK_ACTIVE)
 			continue;
 
 		ksmbd_debug(CONN, "Cancel async request id %d\n",
 			    work->async_id);
-		work->state = KSMBD_WORK_CANCELLED;
 		if (work->cancel_fn)
 			work->cancel_fn(work->cancel_argv);
 	}
@@ -473,6 +474,9 @@ int ksmbd_conn_wait_idle_sess(struct ksmbd_conn *curr_conn,
 	if (retry_count >= max_timeout)
 		return -EIO;
 
+	/* A blocked byte-range lock cannot drain until teardown wakes it. */
+	ksmbd_wake_session_blocked_works(sess);
+
 	down_read(&conn_list_lock);
 	hash_for_each(conn_list, bkt, conn, hlist) {
 		if (ksmbd_session_is_bound_to_conn(sess, conn)) {
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`cancel_fn`
- 外部函数：`down_read`
- 外部函数：`ksmbd_debug`
- 外部函数：`rcu_read_lock`
- 外部函数：`rcu_read_unlock`
- 外部函数：`spin_lock`
- 外部函数：`up_read`
- 外部函数：`xa_load`
- 大写宏：`CONN`
- 大写宏：`EIO`
- 大写宏：`KSMBD_WORK_ACTIVE`
- 大写宏：`KSMBD_WORK_CANCELLED`
- 外部类型：`Cancel`
- 外部类型：`ksmbd_conn`
- 外部类型：`ksmbd_session`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-cfe13f1983` → 本草稿移入 `cases/defect/auto-linux-cfe13f1983/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
