# auto-linux-2424665d0e

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#864c07de0c7dcbca58b61625dfa2e461ae63b242](https://github.com/torvalds/linux/commit/864c07de0c7dcbca58b61625dfa2e461ae63b242) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-24 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 11 |
| 编译错误数（gcc syntax-only） | 21（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #864c07de0c7dcbca58b61625dfa2e461ae63b242 (https://github.com/torvalds/linux/commit/864c07de0c7dcbca58b61625dfa2e461ae63b242)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 239；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 864c07de0c7dcbca58b61625dfa2e461ae63b242 Merge tag 'hardening-v7.4-rc1' of git://git.kernel.org/pub/scm/linux/kernel/git/kees/linux :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -236,19 +236,19 @@ static ssize_t lkdtm_debugfs_entry(struct file *f,
 	if (count >= PAGE_SIZE)
 		return -EINVAL;
 
-	buf = (char *)__get_free_page(GFP_KERNEL);
+	buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
 	if (!buf)
 		return -ENOMEM;
 	if (copy_from_user(buf, user_buf, count)) {
-		free_page((unsigned long) buf);
+		kfree(buf);
 		return -EFAULT;
 	}
 	/* NULL-terminate and remove enter */
 	buf[count] = '\0';
 	strim(buf);
 
 	crashtype = find_crashtype(buf);
-	free_page((unsigned long)buf);
+	kfree(buf);
 
 	if (!crashtype)
 		return -EINVAL;
@@ -271,7 +271,7 @@ static ssize_t lkdtm_debugfs_read(struct file *f, char __user *user_buf,
 	ssize_t out;
 	char *buf;
 
-	buf = (char *)__get_free_page(GFP_KERNEL);
+	buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
 	if (buf == NULL)
 		return -ENOMEM;
 
@@ -290,7 +290,7 @@ static ssize_t lkdtm_debugfs_read(struct file *f, char __user *user_buf,
 
 	out = simple_read_from_buffer(user_buf, count, off,
 				      buf, n);
-	free_page((unsigned long) buf);
+	kfree(buf);
 
 	return out;
 }
@@ -313,19 +313,19 @@ static ssize_t direct_entry(struct file *f, const char __user *user_buf,
 	if (count < 1)
 		return -EINVAL;
 
-	buf = (char *)__get_free_page(GFP_KERNEL);
+	buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
 	if (!buf)
 		return -ENOMEM;
 	if (copy_from_user(buf, user_buf, count)) {
-		free_page((unsigned long) buf);
+		kfree(buf);
 		return -EFAULT;
 	}
 	/* NULL-terminate and remove enter */
 	buf[count] = '\0';
 	strim(buf);
 
 	crashtype = find_crashtype(buf);
-	free_page((unsigned long) buf);
+	kfree(buf);
 	if (!crashtype)
 		return -EINVAL;
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__get_free_page`
- 外部函数：`copy_from_user`
- 外部函数：`find_crashtype`
- 外部函数：`free_page`
- 外部函数：`simple_read_from_buffer`
- 外部函数：`strim`
- 大写宏：`EFAULT`
- 大写宏：`EINVAL`
- 大写宏：`ENOMEM`
- 大写宏：`GFP_KERNEL`
- 大写宏：`NULL`
- 大写宏：`PAGE_SIZE`
- 外部类型：`ssize_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-2424665d0e` → 本草稿移入 `cases/defect/auto-linux-2424665d0e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
