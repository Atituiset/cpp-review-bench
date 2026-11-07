# auto-linux-2c070e86e1

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#c53d8fc9a7571b63482548a7e5284ef9f025fd3f](https://github.com/torvalds/linux/commit/c53d8fc9a7571b63482548a7e5284ef9f025fd3f) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-07 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 52 |
| 编译错误数（gcc syntax-only） | 58（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #c53d8fc9a7571b63482548a7e5284ef9f025fd3f (https://github.com/torvalds/linux/commit/c53d8fc9a7571b63482548a7e5284ef9f025fd3f)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 3164；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT c53d8fc9a7571b63482548a7e5284ef9f025fd3f Merge tag 'block-7.4-20261106' of git://git.kernel.org/pub/scm/linux/kernel/git/axboe/linux :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3160,8 +3160,15 @@ void blk_mq_submit_bio(struct bio *bio)
 		goto queue_exit;
 	}
 
+	/*
+	 * The block layer cannot generate or verify PI for a dma-buf bio, as
+	 * that walks the data pages.  Only fail the bio if it needs that, not
+	 * if the submitter attached its own integrity payload or if PI
+	 * generation or verification is turned off for the device.
+	 */
 	if (unlikely(op_is_dmabuf(bio->bi_opf) &&
-		     bdev_has_integrity_csum(bio->bi_bdev))) {
+		     bdev_has_integrity_csum(bio->bi_bdev) &&
+		     (bio_integrity_action(bio) & BI_ACT_CHECK))) {
 		bio_endio_status(bio, BLK_STS_NOTSUPP);
 		goto queue_exit;
 	}
@@ -3209,16 +3216,15 @@ void blk_mq_submit_bio(struct bio *bio)
 
 	blk_mq_bio_to_request(rq, bio, nr_segs);
 
+	if (bio_zone_write_plugging(bio))
+		blk_zone_write_plug_init_request(rq);
+
 	ret = blk_crypto_rq_get_keyslot(rq);
 	if (ret != BLK_STS_OK) {
-		bio_endio_status(bio, ret);
-		blk_mq_free_request(rq);
+		blk_mq_end_request(rq, ret);
 		return;
 	}
 
-	if (bio_zone_write_plugging(bio))
-		blk_zone_write_plug_init_request(rq);
-
 	if (op_is_flush(bio->bi_opf) && blk_insert_flush(rq))
 		return;
 
@@ -3501,8 +3507,10 @@ void blk_mq_free_rqs(struct blk_mq_tag_set *set, struct blk_mq_tags *tags,
 
 	if (blk_mq_is_shared_tags(set->flags))
 		drv_tags = set->shared_tags;
-	else
+	else if (hctx_idx < set->nr_hw_queues)
 		drv_tags = set->tags[hctx_idx];
+	else
+		drv_tags = NULL;
 
 	if (tags->static_rqs && set->ops->exit_request) {
 		int i;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bdev_has_integrity_csum`
- 外部函数：`bdev_inc_in_flight`
- 外部函数：`bio_endio_status`
- 外部函数：`bio_integrity`
- 外部函数：`bio_zone_write_plugging`
- 外部函数：`blk_crypto_free_request`
- 外部函数：`blk_crypto_rq_bio_prep`
- 外部函数：`blk_crypto_rq_get_keyslot`
- 外部函数：`blk_insert_flush`
- 外部函数：`blk_mq_dec_active_requests`
- 外部函数：`blk_mq_is_shared_tags`
- 外部函数：`blk_mq_put_tag`
- 外部函数：`blk_mq_sched_restart`
- 外部函数：`blk_pm_mark_last_busy`
- 外部函数：`blk_queue_exit`
- 外部函数：`blk_queue_io_stat`
- 外部函数：`blk_rq_count_integrity_sg`
- 外部函数：`blk_rq_is_passthrough`
- 外部函数：`blk_rq_passthrough_stats`
- 外部函数：`blk_time_get_ns`
- 外部函数：`blk_zone_finish_request`
- 外部函数：`blk_zone_write_plug_init_request`
- 外部函数：`finish_request`
- 外部函数：`op_is_dmabuf`
- 外部函数：`op_is_flush`
- 外部函数：`part_stat_lock`
- 外部函数：`part_stat_unlock`
- 外部函数：`req_op`
- 外部函数：`req_ref_put_and_test`
- 外部函数：`rq_qos_done`
- 外部函数：`trace_block_io_start`
- 外部函数：`unlikely`
- 外部函数：`update_io_ticks`
- 大写宏：`BLK_MQ_NO_TAG`
- 大写宏：`BLK_STS_NOTSUPP`
- 大写宏：`BLK_STS_OK`
- 大写宏：`GFP_NOIO`
- 大写宏：`MQ_RQ_IDLE`
- 大写宏：`NULL`
- 大写宏：`REQ_FAILFAST_MASK`
- 大写宏：`REQ_RAHEAD`
- 大写宏：`RQF_IO_STAT`
- 大写宏：`RQF_USE_SCHED`
- 大写宏：`WARN_ON_ONCE`
- 大写宏：`WRITE_ONCE`
- 外部类型：`All`
- 外部类型：`For`
- 外部类型：`This`
- 外部类型：`bio`
- 外部类型：`blk_mq_ctx`
- 外部类型：`blk_mq_hw_ctx`
- 外部类型：`request`
- 外部类型：`request_queue`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-2c070e86e1` → 本草稿移入 `cases/defect/auto-linux-2c070e86e1/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
