# auto-postgres-0c18c4fc5f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#15a947f30726cadff695b7601329d90b6703e268](https://github.com/postgres/postgres/commit/15a947f30726cadff695b7601329d90b6703e268) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-10 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 30 |
| 编译错误数（gcc syntax-only） | 18（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #15a947f30726cadff695b7601329d90b6703e268 (https://github.com/postgres/postgres/commit/15a947f30726cadff695b7601329d90b6703e268)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 17（原始 PR diff 行 415；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 15a947f30726cadff695b7601329d90b6703e268 refactor: Move BackendWritebackContext initialization :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -412,7 +412,6 @@ typedef struct WritebackContext
 /* in buf_init.c */
 extern PGDLLIMPORT BufferDescPadded *BufferDescriptors;
 extern PGDLLIMPORT ConditionVariableMinimallyPadded *BufferIOCVArray;
-extern PGDLLIMPORT WritebackContext BackendWritebackContext;
 
 /* in localbuf.c */
 extern PGDLLIMPORT BufferDesc *LocalBufferDescriptors;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`number`
- 大写宏：`AIO`
- 大写宏：`BM_`
- 大写宏：`BUF_`
- 大写宏：`PGDLLIMPORT`
- 大写宏：`SIZEOF_VOID_P`
- 大写宏：`WRITEBACK_MAX_PENDING_FLUSHES`
- 外部类型：`Backend`
- 外部类型：`BackendWritebackContext`
- 外部类型：`BlockNumber`
- 外部类型：`Buffer`
- 外部类型：`BufferDesc`
- 外部类型：`BufferDescPadded`
- 外部类型：`BufferDescriptors`
- 外部类型：`BufferIOCVArray`
- 外部类型：`BufferTag`
- 外部类型：`ConditionVariableMinimallyPadded`
- 外部类型：`ForkNumber`
- 外部类型：`List`
- 外部类型：`LocalBufferDescriptors`
- 外部类型：`Oid`
- 外部类型：`PGPROCs`
- 外部类型：`PendingWriteback`
- 外部类型：`PgAioWaitRef`
- 外部类型：`Protected`
- 外部类型：`RelFileNumber`
- 外部类型：`See`
- 外部类型：`State`
- 外部类型：`The`
- 外部类型：`WritebackContext`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-0c18c4fc5f` → 本草稿移入 `cases/defect/auto-postgres-0c18c4fc5f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
