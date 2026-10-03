# auto-linux-c3cff00fce

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#3b7cab693ba2bab63774bf5b988e8a61b2ef0f32](https://github.com/torvalds/linux/commit/3b7cab693ba2bab63774bf5b988e8a61b2ef0f32) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-03 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 106 |
| 编译错误数（gcc syntax-only） | 82（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #3b7cab693ba2bab63774bf5b988e8a61b2ef0f32 (https://github.com/torvalds/linux/commit/3b7cab693ba2bab63774bf5b988e8a61b2ef0f32)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 2108；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 3b7cab693ba2bab63774bf5b988e8a61b2ef0f32 Merge tag 'block-7.3-20261002' of git://git.kernel.org/pub/scm/linux/kernel/git/axboe/linux :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2091,6 +2091,7 @@ static inline struct blkcg_gq *blkg_tryget_closest(struct bio *bio,
 	struct request_queue *q = bio->bi_bdev->bd_queue;
 	struct blkcg *blkcg = css_to_blkcg(css);
 	struct blkcg_gq *blkg;
+	unsigned long flags;
 
 	rcu_read_lock();
 	blkg = blkg_lookup(blkcg, q);
@@ -2105,11 +2106,11 @@ static inline struct blkcg_gq *blkg_tryget_closest(struct bio *bio,
 	 * Fast path failed, we're probably issuing IO in this cgroup the first
 	 * time, hold lock to create new blkg.
 	 */
-	spin_lock_irq(&q->queue_lock);
+	spin_lock_irqsave(&q->queue_lock, flags);
 	blkg = blkg_lookup_create(blkcg, bio->bi_bdev->bd_disk);
 	if (blkg)
 		blkg = blkg_lookup_tryget(blkg);
-	spin_unlock_irq(&q->queue_lock);
+	spin_unlock_irqrestore(&q->queue_lock, flags);
 
 	return blkg;
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__blkcg_rstat_flush`
- 外部函数：`alloc_percpu_gfp`
- 外部函数：`bio_list_empty`
- 外部函数：`bio_list_init`
- 外部函数：`bio_list_merge_init`
- 外部函数：`bio_list_pop`
- 外部函数：`blk_finish_plug`
- 外部函数：`blk_get_queue`
- 外部函数：`blk_put_queue`
- 外部函数：`blk_queue_dying`
- 外部函数：`blk_start_plug`
- 外部函数：`blkcg_deactivate_policy`
- 外部函数：`blkcg_policy_enabled`
- 外部函数：`blkg_destroy`
- 外部函数：`blkg_get`
- 外部函数：`blkg_lookup`
- 外部函数：`blkg_put`
- 外部函数：`call_rcu`
- 外部函数：`container_of`
- 外部函数：`css_put`
- 外部函数：`css_to_blkcg`
- 外部函数：`css_tryget_online`
- 外部函数：`free_percpu`
- 外部函数：`hlist_add_head_rcu`
- 外部函数：`kfree`
- 外部函数：`kzalloc_node`
- 外部函数：`likely`
- 外部函数：`list_del_init`
- 外部函数：`lockdep_assert_held`
- 外部函数：`mutex_lock`
- 外部函数：`mutex_unlock`
- 外部函数：`pd_alloc_fn`
- 外部函数：`pd_free_fn`
- 外部函数：`pd_init_fn`
- 外部函数：`per_cpu_ptr`
- 外部函数：`percpu_ref_exit`
- 外部函数：`percpu_ref_init`
- 外部函数：`radix_tree_insert`
- 外部函数：`schedule_work`
- 外部函数：`spin_lock`
- 外部函数：`spin_lock_init`
- 外部函数：`spin_lock_irq`
- 外部函数：`spin_unlock`
- 外部函数：`spin_unlock_irq`
- 外部函数：`submit_bio`
- 外部函数：`u64_stats_init`
- 外部函数：`unlikely`
- 大写宏：`BIO_EMPTY_LIST`
- 大写宏：`BLKCG_MAX_POLS`
- 大写宏：`CONFIG_BLK_CGROUP_PUNT_BIO`
- 大写宏：`ENODEV`
- 大写宏：`ENOMEM`
- 大写宏：`GFP_NOWAIT`
- 大写宏：`INIT_LIST_HEAD`
- 大写宏：`INIT_WORK`
- 大写宏：`NULL`
- 大写宏：`WARN_ON`
- 大写宏：`WARN_ON_ONCE`
- 外部类型：`Both`
- 外部类型：`Flush`
- 外部类型：`Release`
- 外部类型：`bio`
- 外部类型：`bio_list`
- 外部类型：`blk_plug`
- 外部类型：`blkcg`
- 外部类型：`blkcg_gq`
- 外部类型：`blkcg_policy`
- 外部类型：`blkg_iostat_set`
- 外部类型：`blkg_policy_data`
- 外部类型：`gendisk`
- 外部类型：`gfp_t`
- 外部类型：`percpu_ref`
- 外部类型：`rcu_head`
- 外部类型：`request_queue`
- 外部类型：`work_struct`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-c3cff00fce` → 本草稿移入 `cases/defect/auto-linux-c3cff00fce/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
