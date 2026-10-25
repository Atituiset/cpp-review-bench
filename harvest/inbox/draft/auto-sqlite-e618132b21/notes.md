# auto-sqlite-e618132b21

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#30be364a7c4fd8e30249ceea7b5054f134656e02](https://github.com/sqlite/sqlite/commit/30be364a7c4fd8e30249ceea7b5054f134656e02) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-25 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 128 |
| 编译错误数（gcc syntax-only） | 56（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #30be364a7c4fd8e30249ceea7b5054f134656e02 (https://github.com/sqlite/sqlite/commit/30be364a7c4fd8e30249ceea7b5054f134656e02)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 30be364a7c4fd8e30249ceea7b5054f134656e02 Always report back the output flags from the xOpen method of the unix VFS. :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -6646,6 +6646,9 @@ static int unixOpen(
     if( zName
      && (fd = robust_open(zName, O_RDWR|O_EXCL|O_TMPFILE, 0600))>=0
     ){
+      if( pOutFlags ){
+        *pOutFlags = flags;
+      }
       rc = fillInUnixFile(pVfs, fd, pFile, zPath, ctrlFlags|UNIXFILE_NOLOCK);
       goto open_finished;
     }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`defined`
- 外部函数：`int`
- 外部函数：`memcmp`
- 外部函数：`memcpy`
- 外部函数：`osClose`
- 外部函数：`osFchmod`
- 外部函数：`osFcntl`
- 外部函数：`osFstat`
- 外部函数：`osStat`
- 外部函数：`osUnlink`
- 外部函数：`osWrite`
- 外部函数：`robust_open`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_log`
- 外部函数：`sqlite3_malloc64`
- 外部函数：`sqlite3_mutex_enter`
- 外部函数：`sqlite3_mutex_held`
- 外部函数：`sqlite3_mutex_leave`
- 外部函数：`sqlite3_mutex_notheld`
- 外部函数：`ssize_t`
- 外部函数：`strlen`
- 大写宏：`EINTR`
- 大写宏：`FD_CLOEXEC`
- 大写宏：`F_GETFD`
- 大写宏：`F_SETFD`
- 大写宏：`OS_VXWORKS`
- 大写宏：`O_CLOEXEC`
- 大写宏：`O_CREAT`
- 大写宏：`O_EXCL`
- 大写宏：`O_RDONLY`
- 大写宏：`SQLITE_IOCAP_POWERSAFE_OVERWRITE`
- 大写宏：`SQLITE_WARNING`
- 外部类型：`An`
- 外部类型：`Android`
- 外部类型：`Canonical`
- 外部类型：`Connections`
- 外部类型：`Delete`
- 外部类型：`Device`
- 外部类型：`Do`
- 外部类型：`Filename`
- 外部类型：`For`
- 外部类型：`IDs`
- 外部类型：`If`
- 外部类型：`Length`
- 外部类型：`Next`
- 外部类型：`No`
- 外部类型：`Not`
- 外部类型：`Number`
- 外部类型：`Search`
- 外部类型：`See`
- 外部类型：`To`
- 外部类型：`Unique`
- 外部类型：`We`
- 外部类型：`dev_t`
- 外部类型：`ino_t`
- 外部类型：`mode_t`
- 外部类型：`size_t`
- 外部类型：`stat`
- 外部类型：`unixFile`
- 外部类型：`unixInodeInfo`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-e618132b21` → 本草稿移入 `cases/defect/auto-sqlite-e618132b21/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
