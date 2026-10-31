# auto-linux-6a21cb8ec9

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#8ebbaf8835c62d7541a6226672a6c2e8e6a5428a](https://github.com/torvalds/linux/commit/8ebbaf8835c62d7541a6226672a6c2e8e6a5428a) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-31 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 7 |
| 编译错误数（gcc syntax-only） | 8（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #8ebbaf8835c62d7541a6226672a6c2e8e6a5428a (https://github.com/torvalds/linux/commit/8ebbaf8835c62d7541a6226672a6c2e8e6a5428a)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 619；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 8ebbaf8835c62d7541a6226672a6c2e8e6a5428a Merge tag 'block-7.4-20261029' of git://git.kernel.org/pub/scm/linux/kernel/git/axboe/linux :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -611,17 +611,17 @@ static void blk_mq_free_tags_callback(struct rcu_head *head)
 		kmemleak_free(page_address(page));
 		__free_pages(page, page->private);
 	}
+	/* An iterator may walk the bitmaps for as long as it may see the tags */
+	sbitmap_queue_free(&tags->bitmap_tags);
+	sbitmap_queue_free(&tags->breserved_tags);
 	kfree(tags);
 }
 
 void blk_mq_free_tags(struct blk_mq_tags *tags)
 {
-	sbitmap_queue_free(&tags->bitmap_tags);
-	sbitmap_queue_free(&tags->breserved_tags);
-
 	/* if tags pages is not allocated yet, free tags directly */
 	if (list_empty(&tags->page_list)) {
-		kfree(tags);
+		blk_mq_free_tags_callback(&tags->rcu_head);
 		return;
 	}
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__free_pages`
- 外部函数：`kfree`
- 外部函数：`kmemleak_free`
- 外部函数：`list_empty`
- 外部函数：`page_address`
- 外部函数：`sbitmap_queue_free`
- 外部类型：`blk_mq_tags`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-6a21cb8ec9` → 本草稿移入 `cases/defect/auto-linux-6a21cb8ec9/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
