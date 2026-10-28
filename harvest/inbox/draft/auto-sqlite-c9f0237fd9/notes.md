# auto-sqlite-c9f0237fd9

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#814fd8ef4d42d6567f1613a66f556c8194764d06](https://github.com/sqlite/sqlite/commit/814fd8ef4d42d6567f1613a66f556c8194764d06) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 65 |
| 编译错误数（gcc syntax-only） | 130（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #814fd8ef4d42d6567f1613a66f556c8194764d06 (https://github.com/sqlite/sqlite/commit/814fd8ef4d42d6567f1613a66f556c8194764d06)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 814fd8ef4d42d6567f1613a66f556c8194764d06 Merge branch-3.54 fixes into trunk :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -12031,8 +12031,14 @@ static int vec1ReleaseMethod(sqlite3_vtab *pVtab, int iSavepoint){
 static int vec1RollbackToMethod(sqlite3_vtab *pVtab, int iSavepoint){
   Vec1Tab *pTab = (Vec1Tab*)pVtab;
   UNUSED_PARAMETER(iSavepoint);
+
+  /* Discard any cached writes */
   vec1FinishWriter(pTab, 1);
+
+  /* Drop the in-memory cache of both the config and the model. The 
+  ** database ROLLBACK TO operation may have modified them on disk. */
   vec1UnloadConfig(pTab);
+  vec1UnloadModel(pTab);
   return SQLITE_OK;
 }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`length`
- 外部函数：`memset`
- 外部函数：`min`
- 外部函数：`size_of_array`
- 外部函数：`sqlite3_errmsg`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_prepare_v2`
- 外部函数：`sqlite3_realloc64`
- 外部函数：`sqlite3_vmprintf`
- 外部函数：`sum`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 外部函数：`vec1VtabError`
- 大写宏：`AND`
- 大写宏：`BETWEEN`
- 大写宏：`CONFLICT`
- 大写宏：`DELETE`
- 大写宏：`FROM`
- 大写宏：`INSERT`
- 大写宏：`INTO`
- 大写宏：`LIMIT`
- 大写宏：`ORDER`
- 大写宏：`REPLACE`
- 大写宏：`RETURNING`
- 大写宏：`SELECT`
- 大写宏：`SET`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_OK`
- 大写宏：`UPDATE`
- 大写宏：`VALUES`
- 大写宏：`WHERE`
- 外部类型：`Set`
- 外部类型：`This`
- 外部类型：`True`
- 外部类型：`Used`
- 外部类型：`Vec1Buffer`
- 外部类型：`Vec1ListBuilder`
- 外部类型：`Vec1MetaBuilder`
- 外部类型：`Vec1MetaValue`
- 外部类型：`Vec1Tab`
- 外部类型：`Vec1Writer`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-c9f0237fd9` → 本草稿移入 `cases/defect/auto-sqlite-c9f0237fd9/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
