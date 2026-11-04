# auto-linux-a5786225cc

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#51628610f9187c1c1cfe499d92e066bc22ad9693](https://github.com/torvalds/linux/commit/51628610f9187c1c1cfe499d92e066bc22ad9693) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-04 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 18 |
| 编译错误数（gcc syntax-only） | 19（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #51628610f9187c1c1cfe499d92e066bc22ad9693 (https://github.com/torvalds/linux/commit/51628610f9187c1c1cfe499d92e066bc22ad9693)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 380；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 51628610f9187c1c1cfe499d92e066bc22ad9693 vfs: fix take_dentry_name_snapshot() semantics :: PR 修复动作推断：修复前越界访问（加边界/长度检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -359,58 +359,51 @@ static inline int dname_external(const struct dentry *dentry)
 	return dentry->d_name.name != dentry->d_shortname.string;
 }
 
-static bool __take_dentry_name_snapshot(struct name_snapshot *name,
-					struct dentry *dentry, bool retry)
+static bool try_name_snapshot(struct name_snapshot *name, struct dentry *dentry)
 {
-	unsigned seq;
-	const unsigned char *s;
+	const unsigned char *s = READ_ONCE(dentry->d_name.name);
+	struct external_name *p;
 
-	rcu_read_lock();
-retry:
-	seq = read_seqcount_begin(&dentry->d_seq);
-	s = READ_ONCE(dentry->d_name.name);
 	name->name.hash_len = dentry->d_name.hash_len;
 	name->name.name = name->inline_name.string;
 	if (likely(s == dentry->d_shortname.string)) {
 		name->inline_name = dentry->d_shortname;
-	} else {
-		struct external_name *p;
-		p = container_of(s, struct external_name, name[0]);
-		// get a valid reference
-		if (unlikely(!atomic_inc_not_zero(&p->count))) {
-			if (retry)
-				goto retry;
-			rcu_read_unlock();
-			return false;
-		}
-		name->name.name = s;
+		return true;
 	}
-	if (read_seqcount_retry(&dentry->d_seq, seq)) {
-		release_dentry_name_snapshot(name);
-		goto retry;
+
+	// Try to get a valid external reference
+	//
+	// In case we fail to get a ref, 'name.name' will
+	// point to the inline_name, which we leave as an
+	// empty name. release_dentry_name_snapshot() can
+	// be called regardless.
+	p = container_of(s, struct external_name, name[0]);
+	if (unlikely(!atomic_inc_not_zero(&p->count))) {
+		name->name.len = 0;
+		name->inline_name.string[0] = 0;
+		return false;
 	}
-	rcu_read_unlock();
+
+	// We got a ref and the name is now needs to be
+	// free'd regardless of whether the sequence count
+	// matches or not
+	name->name.name = s;
 	return true;
 }
 
-void take_dentry_name_snapshot(struct name_snapshot *name, struct dentry *dentry)
+bool take_dentry_name_snapshot(struct name_snapshot *name, struct dentry *dentry)
 {
-	__take_dentry_name_snapshot(name, dentry, true);
+	guard(rcu)();
+	for (;;) {
+		unsigned seq = read_seqcount_begin(&dentry->d_seq);
+		bool retval = try_name_snapshot(name, dentry);
+		if (!read_seqcount_retry(&dentry->d_seq, seq))
+			return retval;
+		release_dentry_name_snapshot(name);
+	}
 }
 EXPORT_SYMBOL(take_dentry_name_snapshot);
 
-/*
- * For a caller that holds no reference on @dentry, e.g. in RCU walk.
- * The external name of a dead dentry has a zero refcount forever, so
- * take_dentry_name_snapshot() would retry without end.  Fail instead.
- */
-bool try_take_dentry_name_snapshot(struct name_snapshot *name,
-				   struct dentry *dentry)
-{
-	return __take_dentry_name_snapshot(name, dentry, false);
-}
-EXPORT_SYMBOL(try_take_dentry_name_snapshot);
-
 void release_dentry_name_snapshot(struct name_snapshot *name)
 {
 	if (unlikely(name->name.name != name->inline_name.string)) {
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`atomic_inc_not_zero`
- 外部函数：`container_of`
- 外部函数：`likely`
- 外部函数：`rcu_read_lock`
- 外部函数：`rcu_read_unlock`
- 外部函数：`read_seqcount_begin`
- 外部函数：`read_seqcount_retry`
- 外部函数：`try_take_dentry_name_snapshot`
- 外部函数：`unlikely`
- 大写宏：`EXPORT_SYMBOL`
- 大写宏：`RCU`
- 大写宏：`READ_ONCE`
- 外部类型：`Fail`
- 外部类型：`For`
- 外部类型：`The`
- 外部类型：`dentry`
- 外部类型：`external_name`
- 外部类型：`name_snapshot`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-a5786225cc` → 本草稿移入 `cases/defect/auto-linux-a5786225cc/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
