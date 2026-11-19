# auto-linux-58b24a1b28

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#5158f371faceb185235ba42882e67a840cfd7f48](https://github.com/torvalds/linux/commit/5158f371faceb185235ba42882e67a840cfd7f48) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-19 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 3 |
| 编译错误数（gcc syntax-only） | 12（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #5158f371faceb185235ba42882e67a840cfd7f48 (https://github.com/torvalds/linux/commit/5158f371faceb185235ba42882e67a840cfd7f48)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 106；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 5158f371faceb185235ba42882e67a840cfd7f48 ocfs2: fix kernel BUG in ocfs2_page_mkwrite() when write_end fails :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -103,7 +103,17 @@ static vm_fault_t __ocfs2_page_mkwrite(struct file *file,
 		goto out;
 	}
 	err = ocfs2_write_end_nolock(mapping, pos, len, len, fsdata);
-	BUG_ON(err != len);
+	if (err != len) {
+		/*
+		 * ocfs2_write_end_nolock() keeps the mmap folio locked so
+		 * that it can be returned to the VM with VM_FAULT_LOCKED.
+		 * On failure nobody else will unlock it, so do it here.
+		 */
+		folio_unlock(locked_folio);
+		mlog_errno(err);
+		ret = vmf_error(err);
+		goto out;
+	}
 	ret = VM_FAULT_LOCKED;
 out:
 	return ret;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`ocfs2_write_end_nolock`
- 大写宏：`BUG_ON`
- 大写宏：`VM_FAULT_LOCKED`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-58b24a1b28` → 本草稿移入 `cases/defect/auto-linux-58b24a1b28/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
