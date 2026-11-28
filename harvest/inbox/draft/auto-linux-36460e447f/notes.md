# auto-linux-36460e447f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#7cd8495c0f16275c4f9abe1cef09a7b5fcd6d046](https://github.com/torvalds/linux/commit/7cd8495c0f16275c4f9abe1cef09a7b5fcd6d046) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 16 |
| 编译错误数（gcc syntax-only） | 18（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #7cd8495c0f16275c4f9abe1cef09a7b5fcd6d046 (https://github.com/torvalds/linux/commit/7cd8495c0f16275c4f9abe1cef09a7b5fcd6d046)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 7cd8495c0f16275c4f9abe1cef09a7b5fcd6d046 Merge tag 'io_uring-7.4-20261127' of git://git.kernel.org/pub/scm/linux/kernel/git/axboe/linux :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -157,6 +157,13 @@ static int bpf_io_check_member(const struct btf_type *t,
 	return 0;
 }
 
+static int bpf_io_validate(void *kdata)
+{
+	const struct io_uring_bpf_ops *ops = kdata;
+
+	return ops->loop_step ? 0 : -EINVAL;
+}
+
 static int bpf_io_init_member(const struct btf_type *t,
 			       const struct btf_member *member,
 			       void *kdata, const void *udata)
@@ -269,6 +276,7 @@ static struct bpf_struct_ops bpf_ring_ops = {
 	.check_member = bpf_io_check_member,
 	.init_member = bpf_io_init_member,
 	.init = bpf_io_init,
+	.validate = bpf_io_validate,
 	.cfi_stubs = &io_bpf_ops_stubs,
 	.name = "io_uring_bpf_ops",
 	.owner = THIS_MODULE,
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__btf_member_bit_offset`
- 外部函数：`btf_find_by_name_kind`
- 外部函数：`btf_type_by_id`
- 外部函数：`kfuncs`
- 外部函数：`offsetof`
- 外部函数：`pr_err`
- 外部函数：`register_btf_kfunc_id_set`
- 大写宏：`BPF_PROG_TYPE_STRUCT_OPS`
- 大写宏：`BTF_KIND_STRUCT`
- 大写宏：`EINVAL`
- 大写宏：`NULL`
- 大写宏：`THIS_MODULE`
- 外部类型：`Failed`
- 外部类型：`bpf_prog`
- 外部类型：`btf`
- 外部类型：`btf_member`
- 外部类型：`btf_type`
- 外部类型：`io_uring_bpf_ops`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-36460e447f` → 本草稿移入 `cases/defect/auto-linux-36460e447f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
