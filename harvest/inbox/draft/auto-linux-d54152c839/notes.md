# auto-linux-d54152c839

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#36aad21d50d1af3f6954bb0c988257a908202477](https://github.com/torvalds/linux/commit/36aad21d50d1af3f6954bb0c988257a908202477) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-31 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 35 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #36aad21d50d1af3f6954bb0c988257a908202477 (https://github.com/torvalds/linux/commit/36aad21d50d1af3f6954bb0c988257a908202477)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 373；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 36aad21d50d1af3f6954bb0c988257a908202477 Merge tag 'ksmbd-for-7.4-rc1-part2' of git://git.kernel.org/pub/scm/linux/kernel/git/linkinjeon/smb :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -4,6 +4,7 @@
  */
 
 #include <linux/atomic.h>
+#include <linux/err.h>
 #include <linux/list.h>
 #include <linux/slab.h>
 #include <linux/rwsem.h>
@@ -43,11 +44,6 @@ void ksmbd_session_release_slot(struct ksmbd_conn *conn)
 	atomic64_dec(&conn->session_count);
 }
 
-struct ksmbd_session_rpc {
-	int			id;
-	unsigned int		method;
-};
-
 #ifdef CONFIG_PROC_FS
 
 static const struct ksmbd_const_name ksmbd_sess_cap_const_names[] = {
@@ -301,32 +297,35 @@ static void free_channel_list(struct ksmbd_session *sess)
 	up_write(&sess->chann_lock);
 }
 
-static void __session_rpc_close(struct ksmbd_session *sess,
-				struct ksmbd_session_rpc *entry)
+static void ksmbd_session_rpc_free(struct ksmbd_session_rpc *entry)
+{
+	if (refcount_dec_and_test(&entry->refcount)) {
+		ksmbd_rpc_id_free(entry->id);
+		kvfree(entry->pending);
+		kfree(entry);
+	}
+}
+
+static void __session_rpc_close(struct ksmbd_session_rpc *entry)
 {
 	struct ksmbd_rpc_command *resp;
 
-	resp = ksmbd_rpc_close(sess, entry->id);
+	mutex_lock(&entry->lock);
+	resp = ksmbd_rpc_close(entry);
 	if (!resp)
 		pr_err("Unable to close RPC pipe %d\n", entry->id);
-
 	kvfree(resp);
-	ksmbd_rpc_id_free(entry->id);
-	kfree(entry);
+	mutex_unlock(&entry->lock);
+	ksmbd_session_rpc_free(entry);
 }
 
-static void ksmbd_session_rpc_clear_list(struct ksmbd_session *sess)
+void ksmbd_session_rpc_clear_list(struct ksmbd_session *sess)
 {
 	struct ksmbd_session_rpc *entry;
-	long index;
-
-	down_write(&sess->rpc_lock);
-	xa_for_each(&sess->rpc_handle_list, index, entry) {
-		xa_erase(&sess->rpc_handle_list, index);
-		__session_rpc_close(sess, entry);
-	}
-	up_write(&sess->rpc_lock);
+	unsigned long index;
 
+	xa_for_each(&sess->rpc_handle_list, index, entry)
+		ksmbd_session_rpc_close(sess, index);
 	xa_destroy(&sess->rpc_handle_list);
 }
 
@@ -356,9 +355,9 @@ static int __rpc_method(char *rpc_name)
 
 int ksmbd_session_rpc_open(struct ksmbd_session *sess, char *rpc_name)
 {
-	struct ksmbd_session_rpc *entry, *old;
+	struct ksmbd_session_rpc *entry;
 	struct ksmbd_rpc_command *resp;
-	int method, id;
+	int method, id, ret;
 
 	method = __rpc_method(rpc_name);
 	if (method < 0)
@@ -370,31 +369,29 @@ int ksmbd_session_rpc_open(struct ksmbd_session *sess, char *rpc_name)
 
 	entry->method = method;
 	entry->id = id = ksmbd_ipc_id_alloc();
-	if (id < 0)
-		goto free_entry;
-
-	down_write(&sess->rpc_lock);
-	old = xa_store(&sess->rpc_handle_list, id, entry, KSMBD_DEFAULT_GFP);
-	if (xa_is_err(old)) {
-		up_write(&sess->rpc_lock);
-		goto free_id;
+	if (id < 0) {
+		kfree(entry);
+		return id;
 	}
+	mutex_init(&entry->lock);
+	refcount_set(&entry->refcount, 1);
 
-	resp = ksmbd_rpc_open(sess, id);
+	resp = ksmbd_rpc_open(entry);
 	if (!resp) {
-		xa_erase(&sess->rpc_handle_list, entry->id);
-		up_write(&sess->rpc_lock);
-		goto free_id;
+		ksmbd_session_rpc_free(entry);
+		return -EINVAL;
 	}
+	kvfree(resp);
 
+	down_write(&sess->rpc_lock);
+	ret = xa_err(xa_store(&sess->rpc_handle_list, id, entry,
+			      KSMBD_DEFAULT_GFP));
 	up_write(&sess->rpc_lock);
-	kvfree(resp);
+	if (ret) {
+		__session_rpc_close(entry);
+		return ret;
+	}
 	return id;
-free_id:
-	ksmbd_rpc_id_free(entry->id);
-free_entry:
-	kfree(entry);
-	return -EINVAL;
 }
 
 void ksmbd_session_rpc_close(struct ksmbd_session *sess, int id)
@@ -404,8 +401,53 @@ void ksmbd_session_rpc_close(struct ksmbd_session *sess, int id)
 	down_write(&sess->rpc_lock);
 	entry = xa_erase(&sess->rpc_handle_list, id);
 	if (entry)
-		__session_rpc_close(sess, entry);
+		WRITE_ONCE(entry->closed, true);
 	up_write(&sess->rpc_lock);
+	if (entry)
+		__session_rpc_close(entry);
+}
+
+/* The reference pins the entry while waiting without holding rpc_lock. */
+struct ksmbd_session_rpc *ksmbd_session_rpc_get(struct ksmbd_work *work,
+					     int id)
+{
+	struct ksmbd_session_rpc *entry;
+	int err;
+
+	down_read(&work->sess->rpc_lock);
+	entry = xa_load(&work->sess->rpc_handle_list, id);
+	if (entry)
+		refcount_inc(&entry->refcount);
+	up_read(&w
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`atomic64_dec`
- 外部函数：`atomic_inc`
- 外部函数：`down_write`
- 外部函数：`kfree`
- 外部函数：`ksmbd_all_conn_set_status`
- 外部函数：`ksmbd_ipc_id_alloc`
- 外部函数：`ksmbd_rpc_close`
- 外部函数：`ksmbd_rpc_id_free`
- 外部函数：`ksmbd_rpc_open`
- 外部函数：`kvfree`
- 外部函数：`lockdep_assert_held`
- 外部函数：`pr_err`
- 外部函数：`strcmp`
- 外部函数：`up_write`
- 外部函数：`xa_destroy`
- 外部函数：`xa_erase`
- 外部函数：`xa_is_err`
- 外部函数：`xa_load`
- 外部函数：`xa_store`
- 大写宏：`CONFIG_PROC_FS`
- 大写宏：`EINVAL`
- 大写宏：`ENOENT`
- 大写宏：`KSMBD_DEFAULT_GFP`
- 大写宏：`KSMBD_RPC_LSARPC_METHOD_INVOKE`
- 大写宏：`KSMBD_RPC_RAP_METHOD`
- 大写宏：`KSMBD_RPC_SAMR_METHOD_INVOKE`
- 大写宏：`KSMBD_RPC_SRVSVC_METHOD_INVOKE`
- 大写宏：`KSMBD_RPC_WKSSVC_METHOD_INVOKE`
- 大写宏：`KSMBD_SESS_GOOD`
- 大写宏：`LANMAN`
- 大写宏：`RPC`
- 外部类型：`Unable`
- 外部类型：`Unsupported`
- 外部类型：`ksmbd_const_name`
- 外部类型：`ksmbd_rpc_command`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-d54152c839` → 本草稿移入 `cases/defect/auto-linux-d54152c839/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
