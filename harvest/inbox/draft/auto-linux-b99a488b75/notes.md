# auto-linux-b99a488b75

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#62b13f41be6284250c06ab4d79a7fb13fe93f4d7](https://github.com/torvalds/linux/commit/62b13f41be6284250c06ab4d79a7fb13fe93f4d7) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-22 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 11 |
| 编译错误数（gcc syntax-only） | 5（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #62b13f41be6284250c06ab4d79a7fb13fe93f4d7 (https://github.com/torvalds/linux/commit/62b13f41be6284250c06ab4d79a7fb13fe93f4d7)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 4901；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 62b13f41be6284250c06ab4d79a7fb13fe93f4d7 Merge tag 'cifs-fixes-7.4-rc4' of https://git.manguebit.org/linux :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -4819,6 +4819,8 @@ smb2_readv_callback(struct TCP_Server_Info *server, struct mid_q_entry *mid)
 					      rc);
 				rdata->subreq.error = rc;
 				rdata->result = rc;
+				/* the data can't be trusted, don't count it */
+				rdata->got_bytes = 0;
 
 				if (is_replayable_error(rc)) {
 					trace_netfs_sreq(&rdata->subreq, netfs_sreq_trace_io_retry_needed);
@@ -4898,9 +4900,8 @@ smb2_readv_callback(struct TCP_Server_Info *server, struct mid_q_entry *mid)
 			__set_bit(NETFS_SREQ_CLEAR_TAIL, &rdata->subreq.flags);
 	} else {
 		size_t trans = rdata->subreq.transferred + rdata->got_bytes;
-		if (trans < rdata->subreq.len &&
+		if (!rdata->result && trans < rdata->subreq.len &&
 		    rdata->subreq.start + trans >= netfs_read_remote_i_size(inode)) {
-			rdata->result = 0;
 			if (rdata->subreq.start + trans >= i_size_read(inode))
 				__set_bit(NETFS_SREQ_HIT_EOF, &rdata->subreq.flags);
 			else
@@ -5181,14 +5182,23 @@ smb2_writev_callback(struct TCP_Server_Info *server, struct mid_q_entry *mid)
 		if (written > wdata->subreq.len)
 			written &= 0xFFFF;
 
+		/*
+		 * The server must not report more bytes than were sent.  netfs
+		 * uses the subrequest length to work out which folios this
+		 * write has finished with, so it must not be changed here.
+		 */
+		if (written > wdata->subreq.len) {
+			result = smb_EIO2(smb_eio_trace_write_overlarge,
+					  written, wdata->subreq.len);
+			break;
+		}
+
 		cifs_stats_bytes_written(tcon, written);
 
-		if (written < wdata->subreq.len) {
+		if (written < wdata->subreq.len)
 			result = -ENOSPC;
-		} else if (written > 0) {
-			wdata->subreq.len = written;
+		else if (written > 0)
 			__set_bit(NETFS_SREQ_MADE_PROGRESS, &wdata->subreq.flags);
-		}
 		break;
 	case MID_REQUEST_SUBMITTED:
 		trace_netfs_sreq(&wdata->subreq, netfs_sreq_trace_io_req_submitted);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__set_bit`
- 外部函数：`cifs_stats_bytes_written`
- 外部函数：`i_size_read`
- 外部函数：`is_replayable_error`
- 外部函数：`netfs_read_remote_i_size`
- 外部函数：`trace_netfs_sreq`
- 大写宏：`ENOSPC`
- 大写宏：`MID_REQUEST_SUBMITTED`
- 大写宏：`NETFS_SREQ_CLEAR_TAIL`
- 大写宏：`NETFS_SREQ_HIT_EOF`
- 大写宏：`NETFS_SREQ_MADE_PROGRESS`
- 外部类型：`size_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-b99a488b75` → 本草稿移入 `cases/defect/auto-linux-b99a488b75/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
