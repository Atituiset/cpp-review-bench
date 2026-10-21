# auto-linux-f93891c01c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#0d508d4709837bcc88e314c0533a42d3c55b31e3](https://github.com/torvalds/linux/commit/0d508d4709837bcc88e314c0533a42d3c55b31e3) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-21 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 61 |
| 编译错误数（gcc syntax-only） | 111（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #0d508d4709837bcc88e314c0533a42d3c55b31e3 (https://github.com/torvalds/linux/commit/0d508d4709837bcc88e314c0533a42d3c55b31e3)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 1011；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 0d508d4709837bcc88e314c0533a42d3c55b31e3 Merge tag 'audit-pr-20261016' of git://git.kernel.org/pub/scm/linux/kernel/git/pcmoore/audit :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -196,7 +196,7 @@ int audit_match_class(int class, unsigned int syscall)
 }
 
 #ifdef CONFIG_AUDITSYSCALL
-static inline int audit_match_class_bits(int class, u32 *mask)
+static inline int audit_match_class_bits(int class, const u32 *mask)
 {
 	int i;
 
@@ -208,30 +208,62 @@ static inline int audit_match_class_bits(int class, u32 *mask)
 	return 1;
 }
 
-static int audit_match_signal(struct audit_entry *entry)
+static int audit_match_signal(const struct audit_krule *rule)
 {
-	struct audit_field *arch = entry->rule.arch_f;
+	struct audit_field *arch = rule->arch_f;
 
 	if (!arch) {
 		/* When arch is unspecified, we must check both masks on biarch
 		 * as syscall number alone is ambiguous. */
 		return (audit_match_class_bits(AUDIT_CLASS_SIGNAL,
-					       entry->rule.mask) &&
+					       rule->mask) &&
 			audit_match_class_bits(AUDIT_CLASS_SIGNAL_32,
-					       entry->rule.mask));
+					       rule->mask));
 	}
 
 	switch (audit_classify_arch(arch->val)) {
 	case 0: /* native */
 		return (audit_match_class_bits(AUDIT_CLASS_SIGNAL,
-					       entry->rule.mask));
+					       rule->mask));
 	case 1: /* 32bit on biarch */
 		return (audit_match_class_bits(AUDIT_CLASS_SIGNAL_32,
-					       entry->rule.mask));
+					       rule->mask));
 	default:
 		return 1;
 	}
 }
+
+static bool audit_rule_counts_syscalls(const struct audit_krule *rule)
+{
+	switch (rule->listnr) {
+	case AUDIT_FILTER_USER:
+	case AUDIT_FILTER_EXCLUDE:
+	case AUDIT_FILTER_FS:
+		return false;
+	default:
+		return true;
+	}
+}
+
+void audit_rule_account(const struct audit_krule *rule)
+{
+	lockdep_assert_held(&audit_filter_mutex);
+
+	if (audit_rule_counts_syscalls(rule))
+		audit_n_rules++;
+	if (!audit_match_signal(rule))
+		audit_signals++;
+}
+
+void audit_rule_unaccount(const struct audit_krule *rule)
+{
+	lockdep_assert_held(&audit_filter_mutex);
+
+	if (audit_rule_counts_syscalls(rule))
+		audit_n_rules--;
+	if (!audit_match_signal(rule))
+		audit_signals--;
+}
 #endif
 
 /* Common user-space to kernel rule translation. */
@@ -943,17 +975,6 @@ static inline int audit_add_rule(struct audit_entry *entry)
 	struct audit_tree *tree = entry->rule.tree;
 	struct list_head *list;
 	int err = 0;
-#ifdef CONFIG_AUDITSYSCALL
-	int dont_count = 0;
-
-	/* If any of these, don't count towards total */
-	switch (entry->rule.listnr) {
-	case AUDIT_FILTER_USER:
-	case AUDIT_FILTER_EXCLUDE:
-	case AUDIT_FILTER_FS:
-		dont_count = 1;
-	}
-#endif
 
 	mutex_lock(&audit_filter_mutex);
 	e = audit_find_rule(entry, &list);
@@ -1007,13 +1028,7 @@ static inline int audit_add_rule(struct audit_entry *entry)
 			      &audit_rules_list[entry->rule.listnr]);
 		list_add_tail_rcu(&entry->list, list);
 	}
-#ifdef CONFIG_AUDITSYSCALL
-	if (!dont_count)
-		audit_n_rules++;
-
-	if (!audit_match_signal(entry))
-		audit_signals++;
-#endif
+	audit_rule_account(&entry->rule);
 	mutex_unlock(&audit_filter_mutex);
 
 	return err;
@@ -1025,17 +1040,6 @@ int audit_del_rule(struct audit_entry *entry)
 	struct audit_entry  *e;
 	struct list_head *list;
 	int ret = 0;
-#ifdef CONFIG_AUDITSYSCALL
-	int dont_count = 0;
-
-	/* If any of these, don't count towards total */
-	switch (entry->rule.listnr) {
-	case AUDIT_FILTER_USER:
-	case AUDIT_FILTER_EXCLUDE:
-	case AUDIT_FILTER_FS:
-		dont_count = 1;
-	}
-#endif
 
 	mutex_lock(&audit_filter_mutex);
 	e = audit_find_rule(entry, &list);
@@ -1057,14 +1061,7 @@ int audit_del_rule(struct audit_entry *entry)
 	if (e->rule.exe)
 		audit_remove_mark_rule(&e->rule);
 
-#ifdef CONFIG_AUDITSYSCALL
-	if (!dont_count)
-		audit_n_rules--;
-
-	if (!audit_match_signal(entry))
-		audit_signals--;
-#endif
-
+	audit_rule_unaccount(&e->rule);
 	call_rcu(&e->rcu, audit_free_rule_rcu);
 
 out:
@@ -1427,6 +1424,7 @@ static int update_lsm_rule(struct audit_krule *r)
 			list_del(&r->rlist);
 		list_del_rcu(&entry->list);
 		list_del(&r->list);
+		audit_rule_unaccount(r);
 	} else {
 		if (r->watch || r->tree)
 			list_replace_init(&r->rlist, &nentry->rule.rlist)
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`audit_classify_arch`
- 外部函数：`audit_hash_ino`
- 外部函数：`audit_mark_path`
- 外部函数：`audit_put_watch`
- 外部函数：`audit_remove_mark_rule`
- 外部函数：`audit_tree_path`
- 外部函数：`audit_watch_path`
- 外部函数：`call_rcu`
- 外部函数：`container_of`
- 外部函数：`gid_eq`
- 外部函数：`kfree`
- 外部函数：`list_add_tail_rcu`
- 外部函数：`mutex_lock`
- 外部函数：`mutex_unlock`
- 外部函数：`security_audit_rule_free`
- 外部函数：`strcmp`
- 外部函数：`uid_eq`
- 大写宏：`AUDIT_BITMASK_SIZE`
- 大写宏：`AUDIT_CLASS_SIGNAL`
- 大写宏：`AUDIT_CLASS_SIGNAL_32`
- 大写宏：`AUDIT_DIR`
- 大写宏：`AUDIT_EGID`
- 大写宏：`AUDIT_EUID`
- 大写宏：`AUDIT_EXE`
- 大写宏：`AUDIT_FILTERKEY`
- 大写宏：`AUDIT_FILTER_EXCLUDE`
- 大写宏：`AUDIT_FILTER_FS`
- 大写宏：`AUDIT_FILTER_USER`
- 大写宏：`AUDIT_FSGID`
- 大写宏：`AUDIT_FSUID`
- 大写宏：`AUDIT_GID`
- 大写宏：`AUDIT_INODE_BUCKETS`
- 大写宏：`AUDIT_LOGINUID`
- 大写宏：`AUDIT_OBJ_GID`
- 大写宏：`AUDIT_OBJ_LEV_HIGH`
- 大写宏：`AUDIT_OBJ_LEV_LOW`
- 大写宏：`AUDIT_OBJ_ROLE`
- 大写宏：`AUDIT_OBJ_TYPE`
- 大写宏：`AUDIT_OBJ_UID`
- 大写宏：`AUDIT_OBJ_USER`
- 大写宏：`AUDIT_SGID`
- 大写宏：`AUDIT_SUBJ_CLR`
- 大写宏：`AUDIT_SUBJ_ROLE`
- 大写宏：`AUDIT_SUBJ_SEN`
- 大写宏：`AUDIT_SUBJ_TYPE`
- 大写宏：`AUDIT_SUBJ_USER`
- 大写宏：`AUDIT_SUID`
- 大写宏：`AUDIT_UID`
- 大写宏：`AUDIT_WATCH`
- 大写宏：`CONFIG_AUDITSYSCALL`
- 大写宏：`NULL`
- 外部类型：`Common`
- 外部类型：`If`
- 外部类型：`When`
- 外部类型：`audit_entry`
- 外部类型：`audit_field`
- 外部类型：`audit_krule`
- 外部类型：`audit_tree`
- 外部类型：`list_head`
- 外部类型：`rcu_head`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-f93891c01c` → 本草稿移入 `cases/defect/auto-linux-f93891c01c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
