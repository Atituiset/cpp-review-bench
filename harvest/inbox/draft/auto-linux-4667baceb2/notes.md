# auto-linux-4667baceb2

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#23c62304dde29abe92013d85910d03a77dada35d](https://github.com/torvalds/linux/commit/23c62304dde29abe92013d85910d03a77dada35d) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-15 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 56 |
| 编译错误数（gcc syntax-only） | 105（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #23c62304dde29abe92013d85910d03a77dada35d (https://github.com/torvalds/linux/commit/23c62304dde29abe92013d85910d03a77dada35d)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 675；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 23c62304dde29abe92013d85910d03a77dada35d Merge tag 'for-linus-7.4a-rc3-tag' of git://git.kernel.org/pub/scm/linux/kernel/git/xen/tip :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -670,15 +670,19 @@ static long gntdev_ioctl_map_grant_ref(struct gntdev_priv *priv,
 	mutex_lock(&priv->lock);
 	gntdev_add_map(priv, map);
 	op.index = map->index << PAGE_SHIFT;
+	mutex_unlock(&priv->lock);
 
 	if (copy_to_user(u, &op, sizeof(op)) != 0) {
-		list_del(&map->next);
+		mutex_lock(&priv->lock);
+		map = gntdev_find_map_index(priv, op.index >> PAGE_SHIFT,
+					    op.count);
+		if (map)
+			list_del(&map->next);
 		mutex_unlock(&priv->lock);
 		gntdev_put_map(priv, map);
 		return -EFAULT;
 	}
 
-	mutex_unlock(&priv->lock);
 	return 0;
 }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`atomic_read`
- 外部函数：`atomic_sub_return`
- 外部函数：`copy_to_user`
- 外部函数：`evtchn_put`
- 外部函数：`gnttab_dma_free_pages`
- 外部函数：`gnttab_free_pages`
- 外部函数：`gnttab_mmap`
- 外部函数：`gnttab_unmap_refs_async`
- 外部函数：`kfree`
- 外部函数：`kvfree`
- 外部函数：`list`
- 外部函数：`list_add_tail`
- 外部函数：`list_del`
- 外部函数：`mmu_interval_notifier_remove`
- 外部函数：`mutex_lock`
- 外部函数：`mutex_unlock`
- 外部函数：`negative`
- 外部函数：`notify_remote_via_evtchn`
- 外部函数：`page_to_pfn`
- 外部函数：`pfn_to_kaddr`
- 外部函数：`pr_debug`
- 外部函数：`pr_err`
- 外部函数：`refcount_dec_and_test`
- 外部函数：`refcount_inc`
- 外部函数：`refcount_set`
- 外部函数：`xen_pv_domain`
- 大写宏：`CONFIG_XEN_GRANT_DMA_ALLOC`
- 大写宏：`DEBUG`
- 大写宏：`EFAULT`
- 大写宏：`GNTDEV_DMA_FLAG_COHERENT`
- 大写宏：`INVALID_GRANT_HANDLE`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 大写宏：`PAGE_SHIFT`
- 大写宏：`PAGE_SIZE`
- 大写宏：`UNMAP_NOTIFY_CLEAR_BYTE`
- 大写宏：`UNMAP_NOTIFY_SEND_EVENT`
- 大写宏：`WARN`
- 大写宏：`WARN_ON`
- 外部类型：`All`
- 外部类型：`Check`
- 外部类型：`Decrease`
- 外部类型：`Do`
- 外部类型：`GNTST_okay`
- 外部类型：`Increment`
- 外部类型：`It`
- 外部类型：`No`
- 外部类型：`Nothing`
- 外部类型：`Only`
- 外部类型：`Release`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Unmap`
- 外部类型：`gntab_unmap_queue_data`
- 外部类型：`gntdev_grant_map`
- 外部类型：`gntdev_priv`
- 外部类型：`gnttab_dma_alloc_args`
- 外部类型：`uint8_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-4667baceb2` → 本草稿移入 `cases/defect/auto-linux-4667baceb2/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
