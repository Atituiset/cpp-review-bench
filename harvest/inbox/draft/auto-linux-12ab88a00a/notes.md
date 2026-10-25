# auto-linux-12ab88a00a

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#44388c8f3e22b578c9089df53301ef7e53e7fb01](https://github.com/torvalds/linux/commit/44388c8f3e22b578c9089df53301ef7e53e7fb01) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-25 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 142 |
| 编译错误数（gcc syntax-only） | 109（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #44388c8f3e22b578c9089df53301ef7e53e7fb01 (https://github.com/torvalds/linux/commit/44388c8f3e22b578c9089df53301ef7e53e7fb01)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 44388c8f3e22b578c9089df53301ef7e53e7fb01 Merge tag 'mtd/for-7.4' of git://git.kernel.org/pub/scm/linux/kernel/git/mtd/linux :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -283,6 +283,21 @@ static void fixup_use_write_buffers(struct mtd_info *mtd)
 		pr_debug("Using buffer write method\n");
 		mtd->_write = cfi_amdstd_write_buffers;
 	}
+
+	/*
+	 * The word count of the Write to Buffer command is a single bus
+	 * word per device, so an x8 device can be told to program at most
+	 * 256 bytes however large a buffer it advertises - including when
+	 * several of them are interleaved on a wider bus, where CMD()
+	 * replicates the count into each device's lane and it is truncated
+	 * there.
+	 */
+	if (cfi->device_type == CFI_DEVICETYPE_X8 &&
+	    cfi->cfiq->MaxBufWriteSize > 8) {
+		cfi->cfiq->MaxBufWriteSize = 8;
+		mtd->writebufsize = cfi_interleave(cfi) <<
+				    cfi->cfiq->MaxBufWriteSize;
+	}
 }
 #endif /* !FORCE_WORD_WRITE */
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`cfi_send_gen_cmd`
- 外部函数：`cfi_udelay`
- 外部函数：`error`
- 外部函数：`map_read`
- 外部函数：`map_word_andequal`
- 外部函数：`map_word_bitsset`
- 外部函数：`map_word_equal`
- 外部函数：`map_write`
- 外部函数：`mutex_lock`
- 外部函数：`mutex_unlock`
- 外部函数：`pr_debug`
- 外部函数：`pr_err`
- 外部函数：`printk`
- 外部函数：`put_chip`
- 外部函数：`s`
- 外部函数：`time_after`
- 大写宏：`BIT`
- 大写宏：`CFI_DEVICETYPE_X16`
- 大写宏：`CFI_DEVICETYPE_X8`
- 大写宏：`CFI_MFR_INTEL`
- 大写宏：`CFI_POLL_DQ`
- 大写宏：`CFI_POLL_STATUS_REG`
- 大写宏：`CFI_QUIRK_DQ_TRUE_DATA`
- 大写宏：`CMD`
- 大写宏：`DECLARE_WAITQUEUE`
- 大写宏：`EIO`
- 大写宏：`FL_CFI_QUERY`
- 大写宏：`FL_ERASE_SUSPENDING`
- 大写宏：`FL_ERASING`
- 大写宏：`FL_JEDEC_QUERY`
- 大写宏：`FL_POINT`
- 大写宏：`FL_READY`
- 大写宏：`FL_STATUS`
- 大写宏：`FL_WRITING`
- 大写宏：`FL_XIP_WHILE_ERASING`
- 大写宏：`FORCE_WORD_WRITE`
- 大写宏：`KERN_ERR`
- 大写宏：`M29EW`
- 大写宏：`MERGESTATUS`
- 大写宏：`MTD`
- 大写宏：`NULL`
- 外部类型：`After`
- 外部类型：`Delay`
- 外部类型：`Do`
- 外部类型：`Erase`
- 外部类型：`EraseSuspend`
- 外部类型：`For`
- 外部类型：`Issue`
- 外部类型：`It`
- 外部类型：`Micron`
- 外部类型：`MinorVersion`
- 外部类型：`Nobody`
- 外部类型：`Program`
- 外部类型：`Resolving`
- 外部类型：`Resume`
- 外部类型：`Send`
- 外部类型：`Should`
- 外部类型：`So`
- 外部类型：`SoftwareFeatures`
- 外部类型：`Someone`
- 外部类型：`Suspend`
- 外部类型：`The`
- 外部类型：`Using`
- 外部类型：`Waiting`
- 外部类型：`Worst`
- 外部类型：`cfi_pri_amdstd`
- 外部类型：`cfi_private`
- 外部类型：`flchip`
- 外部类型：`map_info`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-12ab88a00a` → 本草稿移入 `cases/defect/auto-linux-12ab88a00a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
