# auto-linux-2f1d648ded

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
| 外部依赖数（dep_count） | 5 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #36aad21d50d1af3f6954bb0c988257a908202477 (https://github.com/torvalds/linux/commit/36aad21d50d1af3f6954bb0c988257a908202477)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 301；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 36aad21d50d1af3f6954bb0c988257a908202477 Merge tag 'ksmbd-for-7.4-rc1-part2' of git://git.kernel.org/pub/scm/linux/kernel/git/linkinjeon/smb :: PR 修复动作推断：修复前越界访问（加边界/长度检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -296,12 +296,13 @@ char *ksmbd_convert_dir_info_name(struct ksmbd_dir_info *d_info,
 				  int *conv_len)
 {
 	char *conv;
-	int  sz = min(4 * d_info->name_len, PATH_MAX);
 
-	if (!sz)
+	if (d_info->name_len <= 0)
 		return NULL;
 
-	conv = kmalloc(sz, KSMBD_DEFAULT_GFP);
+	/* One UTF-16 code unit per input byte, plus the terminating NUL. */
+	conv = kmalloc_array((size_t)d_info->name_len + 1,
+			     sizeof(__le16), KSMBD_DEFAULT_GFP);
 	if (!conv)
 		return NULL;
 
@@ -310,7 +311,6 @@ char *ksmbd_convert_dir_info_name(struct ksmbd_dir_info *d_info,
 				      d_info->name_len, local_nls, 0);
 	*conv_len *= 2;
 
-	/* We allocate buffer twice bigger than needed. */
 	conv[*conv_len] = 0x00;
 	conv[*conv_len + 1] = 0x00;
 	return conv;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`kmalloc`
- 外部函数：`min`
- 大写宏：`KSMBD_DEFAULT_GFP`
- 大写宏：`NULL`
- 大写宏：`PATH_MAX`
- 外部类型：`We`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-2f1d648ded` → 本草稿移入 `cases/defect/auto-linux-2f1d648ded/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
