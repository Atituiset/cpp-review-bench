# auto-linux-7be4b2915c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#f802b6558eb81bd5888d761a0cdc8afe5fd8c941](https://github.com/torvalds/linux/commit/f802b6558eb81bd5888d761a0cdc8afe5fd8c941) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-22 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 9 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #f802b6558eb81bd5888d761a0cdc8afe5fd8c941 (https://github.com/torvalds/linux/commit/f802b6558eb81bd5888d761a0cdc8afe5fd8c941)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1514；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT f802b6558eb81bd5888d761a0cdc8afe5fd8c941 Merge tag 'xfs-fixes-7.4-rc4' of git://git.kernel.org:/pub/scm/fs/xfs/xfs-linux :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1495,9 +1495,34 @@ static void iomap_write_delalloc_scan(struct inode *inode,
  * mapping_seek_hole_data() will return it, and we will skip the punch because
  * the folio is dirty. THis is incorrect - we always need to punch out delalloc
  * beyond EOF in this case as writeback will never write back and covert that
- * delalloc block beyond EOF. Hence we limit the cached data scan range to EOF,
- * resulting in always punching out the range from the EOF to the end of the
- * range the iomap spans.
+ * delalloc block beyond EOF. Hence we limit the cached data scan range, and
+ * always punch out the range from the end of the scan to the end of the range
+ * the iomap spans.
+ *
+ * The scan cannot simply end at EOF, because EOF is a byte offset and a
+ * delalloc reservation covers whole blocks. If EOF lies inside a block, it
+ * splits that block in two:
+ *
+ *	[block start, EOF)	File data. If it is dirty it will be written
+ *				back, so the block must keep its delalloc
+ *				backing.
+ *	[EOF, block end)	Beyond EOF. By the rule above, this would always
+ *				be punched out.
+ *
+ * Both parts share one reservation, and it cannot be split: a punch of a part
+ * of a block takes away the reservation for the whole block. If the scan ended
+ * at EOF, we would skip the punch for the first part because it is dirty, and
+ * then punch the second part, which takes the reservation away from under the
+ * dirty data in the first part. On XFS, writeback that has lost a COW fork
+ * reservation like this writes in place, over a block that is shared with
+ * another file.
+ *
+ * Hence we treat the EOF block as a whole, and let the cached data over it
+ * decide: the scan range is rounded up to the end of the block that EOF lies
+ * in. If the data over the EOF block is dirty, the scan moves the punch start
+ * past the block. If it is clean, the block is punched like any other block in
+ * the range. Only blocks that start at or beyond EOF are punched
+ * unconditionally. If EOF is block aligned, the round up changes nothing.
  *
  * Intervals are of the form [start_byte, end_byte) (i.e. open ended) because it
  * matches the intervals returned by mapping_seek_hole_data(). i.e. SEEK_DATA
@@ -1511,7 +1536,8 @@ void iomap_write_delalloc_release(struct inode *inode, loff_t start_byte,
 		iomap_punch_t punch)
 {
 	loff_t punch_start_byte = start_byte;
-	loff_t scan_end_byte = min(i_size_read(inode), end_byte);
+	loff_t scan_end_byte = min_t(u64, end_byte,
+			round_up((u64)i_size_read(inode), i_blocksize(inode)));
 
 	/*
 	 * The caller must hold invalidate_lock to avoid races with page faults
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`i_size_read`
- 外部函数：`min`
- 大写宏：`EOF`
- 大写宏：`SEEK_DATA`
- 外部类型：`Hence`
- 外部类型：`Intervals`
- 外部类型：`THis`
- 外部类型：`The`
- 外部类型：`iomap_punch_t`
- 外部类型：`loff_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-7be4b2915c` → 本草稿移入 `cases/defect/auto-linux-7be4b2915c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
