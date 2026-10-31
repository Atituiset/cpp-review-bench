# auto-linux-633d8ba7af

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
| 外部依赖数（dep_count） | 25 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #36aad21d50d1af3f6954bb0c988257a908202477 (https://github.com/torvalds/linux/commit/36aad21d50d1af3f6954bb0c988257a908202477)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 36aad21d50d1af3f6954bb0c988257a908202477 Merge tag 'ksmbd-for-7.4-rc1-part2' of git://git.kernel.org/pub/scm/linux/kernel/git/linkinjeon/smb :: PR 修复动作推断：修复前解引用（疑似缺判空）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -7,6 +7,8 @@
 #define __USER_SESSION_MANAGEMENT_H__
 
 #include <linux/hashtable.h>
+#include <linux/mutex.h>
+#include <linux/refcount.h>
 #include <linux/xarray.h>
 
 #include "../smb_common.h"
@@ -17,6 +19,18 @@
 #define PREAUTH_HASHVALUE_SIZE		64
 
 struct ksmbd_file_table;
+struct ksmbd_rpc_command;
+struct ksmbd_work;
+
+struct ksmbd_session_rpc {
+	int			id;
+	unsigned int		method;
+	struct mutex		lock;
+	refcount_t		refcount;
+	bool			closed;
+	struct ksmbd_rpc_command *pending;
+	unsigned int		pending_offset;
+};
 
 struct channel {
 	char			sess_key[CIFS_KEY_SIZE];
@@ -133,7 +147,11 @@ void ksmbd_release_tree_conn_id(struct ksmbd_session *sess, int id);
 
 int ksmbd_session_rpc_open(struct ksmbd_session *sess, char *rpc_name);
 void ksmbd_session_rpc_close(struct ksmbd_session *sess, int id);
+void ksmbd_session_rpc_clear_list(struct ksmbd_session *sess);
 int ksmbd_session_rpc_method(struct ksmbd_session *sess, int id);
+struct ksmbd_session_rpc *ksmbd_session_rpc_get(struct ksmbd_work *work,
+					     int id);
+void ksmbd_session_rpc_put(struct ksmbd_session_rpc *entry);
 void ksmbd_user_session_get(struct ksmbd_session *sess);
 void ksmbd_user_session_put(struct ksmbd_session *sess);
 int create_proc_sessions(void);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`create_proc_sessions`
- 外部函数：`ksmbd_session_rpc_close`
- 外部函数：`ksmbd_session_rpc_method`
- 外部函数：`ksmbd_session_rpc_open`
- 外部函数：`ksmbd_user_session_get`
- 外部函数：`ksmbd_user_session_put`
- 大写宏：`CIFS_KEY_SIZE`
- 大写宏：`CONFIG_PROC_FS`
- 大写宏：`SMB2_CLIENT_GUID_SIZE`
- 大写宏：`SMB3`
- 大写宏：`SMB3_ENC_DEC_KEY_SIZE`
- 大写宏：`SMB3_SIGN_KEY_SIZE`
- 外部类型：`ClientGUID`
- 外部类型：`Holds`
- 外部类型：`Preauth_HashValue`
- 外部类型：`atomic_t`
- 外部类型：`hlist_node`
- 外部类型：`ida`
- 外部类型：`ksmbd_conn`
- 外部类型：`ksmbd_file_table`
- 外部类型：`ksmbd_user`
- 外部类型：`list_head`
- 外部类型：`proc_dir_entry`
- 外部类型：`rw_semaphore`
- 外部类型：`xarray`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-633d8ba7af` → 本草稿移入 `cases/defect/auto-linux-633d8ba7af/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
