# auto-linux-0842135c5a

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#9d66f26fefacebfeed15de31c1123451a17ff119](https://github.com/torvalds/linux/commit/9d66f26fefacebfeed15de31c1123451a17ff119) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-12 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 114 |
| 编译错误数（gcc syntax-only） | 105（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #9d66f26fefacebfeed15de31c1123451a17ff119 (https://github.com/torvalds/linux/commit/9d66f26fefacebfeed15de31c1123451a17ff119)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 911；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 9d66f26fefacebfeed15de31c1123451a17ff119 Merge tag 'vfs-7.4-rc3.fixes.2' of git://git.kernel.org/pub/scm/linux/kernel/git/vfs/vfs :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -891,45 +891,34 @@ static int pipe_to_sg(struct pipe_inode_info *pipe, struct pipe_buffer *buf,
 {
 	struct sg_list *sgl = sd->u.data;
 	unsigned int offset, len;
+	struct page *page;
+	char *src;
 
 	if (sgl->n == sgl->size)
 		return 0;
 
-	/* Try lock this page */
-	if (pipe_buf_try_steal(pipe, buf)) {
-		/* Get reference and unlock page for moving */
-		get_page(buf->page);
-		unlock_page(buf->page);
-
-		len = min(buf->len, sd->len);
-		sg_set_page(&(sgl->sg[sgl->n]), buf->page, len, buf->offset);
-	} else {
-		/* Failback to copying a page */
-		struct page *page = alloc_page(GFP_KERNEL);
-		char *src;
-
-		if (!page)
-			return -ENOMEM;
+	page = alloc_page(GFP_KERNEL);
+	if (!page)
+		return -ENOMEM;
 
-		offset = sd->pos & ~PAGE_MASK;
+	offset = sd->pos & ~PAGE_MASK;
 
-		len = sd->len;
-		if (len + offset > PAGE_SIZE)
-			len = PAGE_SIZE - offset;
+	len = sd->len;
+	if (len + offset > PAGE_SIZE)
+		len = PAGE_SIZE - offset;
 
-		src = kmap_local_page(buf->page);
-		memcpy(page_address(page) + offset, src + buf->offset, len);
-		kunmap_local(src);
+	src = kmap_local_page(buf->page);
+	memcpy(page_address(page) + offset, src + buf->offset, len);
+	kunmap_local(src);
 
-		sg_set_page(&(sgl->sg[sgl->n]), page, len, offset);
-	}
+	sg_set_page(&sgl->sg[sgl->n], page, len, offset);
 	sgl->n++;
 	sgl->len += len;
 
 	return len;
 }
 
-/* Faster zero-copy write by splicing */
+/* Write by splicing, copying the data from the pipe */
 static ssize_t port_fops_splice_write(struct pipe_inode_info *pipe,
 				      struct file *filp, loff_t *ppos,
 				      size_t len, unsigned int flags)
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__counted_by`
- 外部函数：`dma_alloc_coherent`
- 外部函数：`dma_free_coherent`
- 外部函数：`get_device`
- 外部函数：`kfree`
- 外部函数：`kmalloc`
- 外部函数：`kmalloc_flex`
- 外部函数：`list_add_tail`
- 外部函数：`list_cut_position`
- 外部函数：`list_del`
- 外部函数：`list_empty`
- 外部函数：`put_device`
- 外部函数：`put_page`
- 外部函数：`remove_port_data`
- 外部函数：`remove_vqs`
- 外部函数：`sg_page`
- 外部函数：`smp_load_acquire`
- 外部函数：`smp_store_release`
- 外部函数：`spin_lock_irqsave`
- 外部函数：`spin_unlock_irqrestore`
- 外部函数：`teardown`
- 外部函数：`virtqueue_add_outbuf`
- 外部函数：`virtqueue_get_buf`
- 大写宏：`CONFIG_REMOTEPROC`
- 大写宏：`DMA`
- 大写宏：`IS_ENABLED`
- 大写宏：`LIST_HEAD`
- 大写宏：`NULL`
- 大写宏：`READ_ONCE`
- 大写宏：`VIRTIO_ID_RPROC_SERIAL`
- 外部类型：`Allocate`
- 外部类型：`Array`
- 外部类型：`Check`
- 外部类型：`Create`
- 外部类型：`Device`
- 外部类型：`If`
- 外部类型：`Increase`
- 外部类型：`List`
- 外部类型：`Major`
- 外部类型：`Next`
- 外部类型：`Pairs`
- 外部类型：`Ports`
- 外部类型：`Release`
- 外部类型：`Set`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`To`
- 外部类型：`When`
- 外部类型：`Workqueue`
- 外部类型：`device`
- 外部类型：`dma_addr_t`
- 外部类型：`gfp_t`
- 外部类型：`list_head`
- 外部类型：`page`
- 外部类型：`port`
- 外部类型：`scatterlist`
- 外部类型：`size_t`
- 外部类型：`spinlock_t`
- 外部类型：`ssize_t`
- 外部类型：`virtio_console_control`
- 外部类型：`virtio_device`
- 外部类型：`virtqueue`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-0842135c5a` → 本草稿移入 `cases/defect/auto-linux-0842135c5a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
