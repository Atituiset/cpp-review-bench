# auto-sqlite-e1c8c2e692

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#ef3e33c3df1cebefb98f3eb1b9e83112121c1c3a](https://github.com/sqlite/sqlite/commit/ef3e33c3df1cebefb98f3eb1b9e83112121c1c3a) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-16 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 68 |
| 编译错误数（gcc syntax-only） | 28（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #ef3e33c3df1cebefb98f3eb1b9e83112121c1c3a (https://github.com/sqlite/sqlite/commit/ef3e33c3df1cebefb98f3eb1b9e83112121c1c3a)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT ef3e33c3df1cebefb98f3eb1b9e83112121c1c3a Fix DEFAULT so that BLOB values cast to text use the database encoding. :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1930,6 +1930,7 @@ static int valueFromExpr(
     assert( zVal[nVal]=='\'' );
     sqlite3VdbeMemSetStr(pVal, sqlite3HexToBlob(db, zVal, nVal), nVal/2,
                          0, SQLITE_DYNAMIC);
+    pVal->enc = enc;
   }
 #endif
 #ifdef SQLITE_ENABLE_STAT4
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`sqlite3DbFreeNN`
- 外部函数：`sqlite3DbMallocRaw`
- 外部函数：`sqlite3DbMallocSize`
- 外部函数：`sqlite3DbReallocOrFree`
- 外部函数：`sqlite3Realloc`
- 外部函数：`sqlite3VdbeMemIsRowSet`
- 外部函数：`sqlite3VdbeMemSetNull`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_mutex_held`
- 外部函数：`sqlite3_result_pointer`
- 外部函数：`testcase`
- 外部函数：`xDel`
- 外部函数：`xFinalize`
- 大写宏：`CORRUPT_DB`
- 大写宏：`ENC`
- 大写宏：`IMP`
- 大写宏：`NULL`
- 大写宏：`SQLITE_DYNAMIC`
- 大写宏：`SQLITE_NOINLINE`
- 大写宏：`SQLITE_NOMEM_BKPT`
- 大写宏：`SQLITE_OK`
- 外部类型：`An`
- 外部类型：`Cannot`
- 外部类型：`FuncDef`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`MEM_Agg`
- 外部类型：`MEM_Blob`
- 外部类型：`MEM_Cleared`
- 外部类型：`MEM_Dyn`
- 外部类型：`MEM_Ephem`
- 外部类型：`MEM_FromBind`
- 外部类型：`MEM_Int`
- 外部类型：`MEM_IntReal`
- 外部类型：`MEM_Null`
- 外部类型：`MEM_Real`
- 外部类型：`MEM_Static`
- 外部类型：`MEM_Str`
- 外部类型：`MEM_Subtype`
- 外部类型：`MEM_Term`
- 外部类型：`MEM_Undefined`
- 外部类型：`Mem`
- 外部类型：`Memory`
- 外部类型：`NULLs`
- 外部类型：`No`
- 外部类型：`That`
- 外部类型：`The`
- 外部类型：`There`
- 外部类型：`This`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-e1c8c2e692` → 本草稿移入 `cases/defect/auto-sqlite-e1c8c2e692/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
