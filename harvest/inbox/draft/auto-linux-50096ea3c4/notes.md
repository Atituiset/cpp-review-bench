# auto-linux-50096ea3c4

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#ca62fc8c6b1841039aee6de1ef03c9863751ec04](https://github.com/torvalds/linux/commit/ca62fc8c6b1841039aee6de1ef03c9863751ec04) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-01 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 9 |
| 编译错误数（gcc syntax-only） | 12（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #ca62fc8c6b1841039aee6de1ef03c9863751ec04 (https://github.com/torvalds/linux/commit/ca62fc8c6b1841039aee6de1ef03c9863751ec04)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 379；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT ca62fc8c6b1841039aee6de1ef03c9863751ec04 Merge tag 'ovl-fixes-7.3' of git://git.kernel.org/pub/scm/linux/kernel/git/overlayfs/vfs :: PR 修复动作推断：修复前越界访问（加边界/长度检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -359,7 +359,8 @@ static inline int dname_external(const struct dentry *dentry)
 	return dentry->d_name.name != dentry->d_shortname.string;
 }
 
-void take_dentry_name_snapshot(struct name_snapshot *name, struct dentry *dentry)
+static bool __take_dentry_name_snapshot(struct name_snapshot *name,
+					struct dentry *dentry, bool retry)
 {
 	unsigned seq;
 	const unsigned char *s;
@@ -376,18 +377,40 @@ void take_dentry_name_snapshot(struct name_snapshot *name, struct dentry *dentry
 		struct external_name *p;
 		p = container_of(s, struct external_name, name[0]);
 		// get a valid reference
-		if (unlikely(!atomic_inc_not_zero(&p->count)))
-			goto retry;
+		if (unlikely(!atomic_inc_not_zero(&p->count))) {
+			if (retry)
+				goto retry;
+			rcu_read_unlock();
+			return false;
+		}
 		name->name.name = s;
 	}
 	if (read_seqcount_retry(&dentry->d_seq, seq)) {
 		release_dentry_name_snapshot(name);
 		goto retry;
 	}
 	rcu_read_unlock();
+	return true;
+}
+
+void take_dentry_name_snapshot(struct name_snapshot *name, struct dentry *dentry)
+{
+	__take_dentry_name_snapshot(name, dentry, true);
 }
 EXPORT_SYMBOL(take_dentry_name_snapshot);
 
+/*
+ * For a caller that holds no reference on @dentry, e.g. in RCU walk.
+ * The external name of a dead dentry has a zero refcount forever, so
+ * take_dentry_name_snapshot() would retry without end.  Fail instead.
+ */
+bool try_take_dentry_name_snapshot(struct name_snapshot *name,
+				   struct dentry *dentry)
+{
+	return __take_dentry_name_snapshot(name, dentry, false);
+}
+EXPORT_SYMBOL(try_take_dentry_name_snapshot);
+
 void release_dentry_name_snapshot(struct name_snapshot *name)
 {
 	if (unlikely(name->name.name != name->inline_name.string)) {
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`atomic_inc_not_zero`
- 外部函数：`container_of`
- 外部函数：`rcu_read_unlock`
- 外部函数：`read_seqcount_retry`
- 外部函数：`unlikely`
- 大写宏：`EXPORT_SYMBOL`
- 外部类型：`dentry`
- 外部类型：`external_name`
- 外部类型：`name_snapshot`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-50096ea3c4` → 本草稿移入 `cases/defect/auto-linux-50096ea3c4/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
