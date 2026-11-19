# auto-linux-0ea40facdf

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#4405c18c95d2d0f99f6c581ef0ee8591a1aa6ccd](https://github.com/torvalds/linux/commit/4405c18c95d2d0f99f6c581ef0ee8591a1aa6ccd) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-19 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 13 |
| 编译错误数（gcc syntax-only） | 11（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #4405c18c95d2d0f99f6c581ef0ee8591a1aa6ccd (https://github.com/torvalds/linux/commit/4405c18c95d2d0f99f6c581ef0ee8591a1aa6ccd)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 3099；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 4405c18c95d2d0f99f6c581ef0ee8591a1aa6ccd Merge tag 'mm-nonmm-hotfixes-stable-2026-11-18-00-06' of git://git.kernel.org/pub/scm/linux/kernel/git/akpm/mm :: PR 修复动作推断：修复前解引用（疑似缺判空）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3094,9 +3094,17 @@ static void *ocfs2_dlm_seq_start(struct seq_file *m, loff_t *pos)
 {
 	struct ocfs2_dlm_seq_priv *priv = m->private;
 	struct ocfs2_lock_res *iter;
+	struct ocfs2_lock_res *dummy = &priv->p_iter_res;
+
+	/* Continue or retry the snapshot left by ->next(). */
+	if (*pos)
+		return priv->p_tmp_res.l_ops ? &priv->p_tmp_res : NULL;
 
+	/* Reads from zero and seek traversal start at the list head. */
 	spin_lock(&ocfs2_dlm_tracking_lock);
-	iter = ocfs2_dlm_next_res(&priv->p_iter_res, priv);
+	list_move(&dummy->l_debug_list,
+		  &priv->p_dlm_debug->d_lockres_tracking);
+	iter = ocfs2_dlm_next_res(dummy, priv);
 	if (iter) {
 		/* Since lockres' have the lifetime of their container
 		 * (which can be inodes, ocfs2_supers, etc) we want to
@@ -3105,8 +3113,11 @@ static void *ocfs2_dlm_seq_start(struct seq_file *m, loff_t *pos)
 		 * trust any pointers on the copy returned, but that's
 		 * ok as the information we want isn't typically held
 		 * in them. */
+		list_move(&dummy->l_debug_list, &iter->l_debug_list);
 		priv->p_tmp_res = *iter;
 		iter = &priv->p_tmp_res;
+	} else {
+		priv->p_tmp_res.l_ops = NULL;
 	}
 	spin_unlock(&ocfs2_dlm_tracking_lock);
 
@@ -3120,17 +3131,19 @@ static void ocfs2_dlm_seq_stop(struct seq_file *m, void *v)
 static void *ocfs2_dlm_seq_next(struct seq_file *m, void *v, loff_t *pos)
 {
 	struct ocfs2_dlm_seq_priv *priv = m->private;
-	struct ocfs2_lock_res *iter = v;
+	struct ocfs2_lock_res *iter;
 	struct ocfs2_lock_res *dummy = &priv->p_iter_res;
 
 	(*pos)++;
 	spin_lock(&ocfs2_dlm_tracking_lock);
-	iter = ocfs2_dlm_next_res(iter, priv);
+	iter = ocfs2_dlm_next_res(dummy, priv);
 	list_del_init(&dummy->l_debug_list);
 	if (iter) {
 		list_add(&dummy->l_debug_list, &iter->l_debug_list);
 		priv->p_tmp_res = *iter;
 		iter = &priv->p_tmp_res;
+	} else {
+		priv->p_tmp_res.l_ops = NULL;
 	}
 	spin_unlock(&ocfs2_dlm_tracking_lock);
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert_spin_locked`
- 外部函数：`list_add`
- 外部函数：`list_del_init`
- 外部函数：`mlog`
- 外部函数：`spin_lock`
- 外部函数：`spin_unlock`
- 大写宏：`NULL`
- 外部类型：`End`
- 外部类型：`Since`
- 外部类型：`We`
- 外部类型：`loff_t`
- 外部类型：`ocfs2_dlm_debug`
- 外部类型：`ocfs2_lock_res`
- 外部类型：`seq_file`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-0ea40facdf` → 本草稿移入 `cases/defect/auto-linux-0ea40facdf/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
