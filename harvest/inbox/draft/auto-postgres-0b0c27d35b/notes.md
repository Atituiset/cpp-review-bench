# auto-postgres-0b0c27d35b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#01b3d24c41629c577ca172f3077ec7bfc2ca9bf3](https://github.com/postgres/postgres/commit/01b3d24c41629c577ca172f3077ec7bfc2ca9bf3) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-03 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 8 |
| 编译错误数（gcc syntax-only） | 26（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #01b3d24c41629c577ca172f3077ec7bfc2ca9bf3 (https://github.com/postgres/postgres/commit/01b3d24c41629c577ca172f3077ec7bfc2ca9bf3)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1891；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 01b3d24c41629c577ca172f3077ec7bfc2ca9bf3 Fix incorrect LSNs in WAL decoding error messages :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1888,7 +1888,7 @@ DecodeXLogRecord(XLogReaderState *state,
 				report_invalid_record(state,
 									  "out-of-order block_id %u at %X/%08X",
 									  block_id,
-									  LSN_FORMAT_ARGS(state->ReadRecPtr));
+									  LSN_FORMAT_ARGS(lsn));
 				goto err;
 			}
 			decoded->max_block_id = block_id;
@@ -1911,15 +1911,15 @@ DecodeXLogRecord(XLogReaderState *state,
 			{
 				report_invalid_record(state,
 									  "BKPBLOCK_HAS_DATA set, but no data included at %X/%08X",
-									  LSN_FORMAT_ARGS(state->ReadRecPtr));
+									  LSN_FORMAT_ARGS(lsn));
 				goto err;
 			}
 			if (!blk->has_data && blk->data_len != 0)
 			{
 				report_invalid_record(state,
 									  "BKPBLOCK_HAS_DATA not set, but data length is %d at %X/%08X",
 									  blk->data_len,
-									  LSN_FORMAT_ARGS(state->ReadRecPtr));
+									  LSN_FORMAT_ARGS(lsn));
 				goto err;
 			}
 			datatotal += blk->data_len;
@@ -1957,7 +1957,7 @@ DecodeXLogRecord(XLogReaderState *state,
 										  blk->hole_offset,
 										  blk->hole_length,
 										  blk->bimg_len,
-										  LSN_FORMAT_ARGS(state->ReadRecPtr));
+										  LSN_FORMAT_ARGS(lsn));
 					goto err;
 				}
 
@@ -1972,7 +1972,7 @@ DecodeXLogRecord(XLogReaderState *state,
 										  "BKPIMAGE_HAS_HOLE not set, but hole offset %d length %d at %X/%08X",
 										  blk->hole_offset,
 										  blk->hole_length,
-										  LSN_FORMAT_ARGS(state->ReadRecPtr));
+										  LSN_FORMAT_ARGS(lsn));
 					goto err;
 				}
 
@@ -1985,7 +1985,7 @@ DecodeXLogRecord(XLogReaderState *state,
 					report_invalid_record(state,
 										  "BKPIMAGE_COMPRESSED set, but block image length %d at %X/%08X",
 										  blk->bimg_len,
-										  LSN_FORMAT_ARGS(state->ReadRecPtr));
+										  LSN_FORMAT_ARGS(lsn));
 					goto err;
 				}
 
@@ -2000,7 +2000,7 @@ DecodeXLogRecord(XLogReaderState *state,
 					report_invalid_record(state,
 										  "neither BKPIMAGE_HAS_HOLE nor BKPIMAGE_COMPRESSED set, but block image length is %d at %X/%08X",
 										  blk->bimg_len,
-										  LSN_FORMAT_ARGS(state->ReadRecPtr));
+										  LSN_FORMAT_ARGS(lsn));
 					goto err;
 				}
 			}
@@ -2015,7 +2015,7 @@ DecodeXLogRecord(XLogReaderState *state,
 				{
 					report_invalid_record(state,
 										  "BKPBLOCK_SAME_REL set but no previous rel at %X/%08X",
-										  LSN_FORMAT_ARGS(state->ReadRecPtr));
+										  LSN_FORMAT_ARGS(lsn));
 					goto err;
 				}
 
@@ -2027,7 +2027,7 @@ DecodeXLogRecord(XLogReaderState *state,
 		{
 			report_invalid_record(state,
 								  "invalid block_id %u at %X/%08X",
-								  block_id, LSN_FORMAT_ARGS(state->ReadRecPtr));
+								  block_id, LSN_FORMAT_ARGS(lsn));
 			goto err;
 		}
 	}
@@ -2094,7 +2094,7 @@ DecodeXLogRecord(XLogReaderState *state,
 shortdata_err:
 	report_invalid_record(state,
 						  "record with invalid length at %X/%08X",
-						  LSN_FORMAT_ARGS(state->ReadRecPtr));
+						  LSN_FORMAT_ARGS(lsn));
 err:
 	*errormsg = state->errormsg_buf;
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`_`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 外部函数：`vsnprintf`
- 大写宏：`BKPBLOCK_HAS_DATA`
- 大写宏：`BKPBLOCK_SAME_REL`
- 大写宏：`BKPIMAGE_COMPRESSED`
- 大写宏：`BKPIMAGE_HAS_HOLE`
- 大写宏：`LSN_FORMAT_ARGS`
- 外部类型：`ReadRecPtr`
- 外部类型：`XLogReaderState`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-0b0c27d35b` → 本草稿移入 `cases/defect/auto-postgres-0b0c27d35b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
