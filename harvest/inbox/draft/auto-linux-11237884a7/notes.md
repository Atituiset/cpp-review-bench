# auto-linux-11237884a7

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
| 外部依赖数（dep_count） | 4 |
| 编译错误数（gcc syntax-only） | 4（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #9a58da80053f992b285b6b7bebc694b0f284c443 (https://github.com/torvalds/linux/commit/9a58da80053f992b285b6b7bebc694b0f284c443)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 672；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 9a58da80053f992b285b6b7bebc694b0f284c443 Merge tag 'ksmbd-for-7.3-rc2' of git://git.kernel.org/pub/scm/linux/kernel/git/linkinjeon/smb :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -666,10 +666,21 @@ void destroy_previous_session(struct ksmbd_conn *conn,
 	    memcmp(user->passkey, prev_user->passkey, user->passkey_sz))
 		goto out;
 
+	down_write(&prev_sess->chann_lock);
+	if (prev_sess->tearing_down) {
+		up_write(&prev_sess->chann_lock);
+		goto out;
+	}
+	prev_sess->tearing_down = true;
+	up_write(&prev_sess->chann_lock);
+
 	ksmbd_all_conn_set_status(prev_sess, KSMBD_SESS_NEED_RECONNECT);
 	err = ksmbd_conn_wait_idle_sess(conn, prev_sess);
 	if (err) {
-		ksmbd_all_conn_set_status(prev_sess, KSMBD_SESS_NEED_SETUP);
+		down_write(&prev_sess->chann_lock);
+		prev_sess->tearing_down = false;
+		up_write(&prev_sess->chann_lock);
+		ksmbd_all_conn_set_status(prev_sess, KSMBD_SESS_GOOD);
 		goto out;
 	}
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`ksmbd_all_conn_set_status`
- 外部函数：`ksmbd_conn_wait_idle_sess`
- 外部函数：`memcmp`
- 大写宏：`KSMBD_SESS_NEED_RECONNECT`
- 大写宏：`KSMBD_SESS_NEED_SETUP`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-linux-11237884a7` → 本草稿移入 `cases/defect/auto-linux-11237884a7/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
