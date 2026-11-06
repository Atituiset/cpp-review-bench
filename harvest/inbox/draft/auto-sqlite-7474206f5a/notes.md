# auto-sqlite-7474206f5a

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#1d41e2da9afcdc94a83d0216caa3e09c1a58ae1c](https://github.com/sqlite/sqlite/commit/1d41e2da9afcdc94a83d0216caa3e09c1a58ae1c) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-06 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 39 |
| 编译错误数（gcc syntax-only） | 60（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1d41e2da9afcdc94a83d0216caa3e09c1a58ae1c (https://github.com/sqlite/sqlite/commit/1d41e2da9afcdc94a83d0216caa3e09c1a58ae1c)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 2751；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 1d41e2da9afcdc94a83d0216caa3e09c1a58ae1c Fix overly aggressive recursion depth limitations in json_pretty(). :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2748,7 +2748,6 @@ static u32 jsonTranslateBlobToPrettyText(
         if( pPretty->nIndent >= JSON_MAX_DEPTH ){
           jsonStringTooDeep(pOut);
         }
-        pParse->iDepth = pPretty->nIndent;
         while( pOut->eErr==0 ){
           jsonPrettyIndent(pPretty);
           j = jsonTranslateBlobToText(pParse, j, pOut);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`jsonConvertInteger`
- 外部函数：`memcpy`
- 外部函数：`sqlite3HexToInt`
- 外部函数：`sqlite3Isdigit`
- 外部函数：`sqlite3Isxdigit`
- 外部函数：`sqlite3RCStrNew`
- 外部函数：`sqlite3RCStrResize`
- 外部函数：`sqlite3RCStrUnref`
- 外部函数：`sqlite3_result_error`
- 外部函数：`sqlite3_result_error_nomem`
- 外部函数：`sqlite3_vsnprintf`
- 外部函数：`strlen`
- 外部函数：`testcase`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 大写宏：`JSON`
- 大写宏：`JSON5`
- 大写宏：`JSONB`
- 大写宏：`NO_TEST`
- 大写宏：`SQL`
- 大写宏：`SQLITE_NOINLINE`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_OK`
- 大写宏：`UTF8`
- 外部类型：`An`
- 外部类型：`Exit`
- 外部类型：`Float`
- 外部类型：`Header`
- 外部类型：`Index`
- 外部类型：`Integer`
- 外部类型：`JsonParse`
- 外部类型：`JsonPretty`
- 外部类型：`JsonString`
- 外部类型：`Malformed`
- 外部类型：`Note`
- 外部类型：`Os`
- 外部类型：`Out`
- 外部类型：`Payload`
- 外部类型：`Quirks`
- 外部类型：`Start`
- 外部类型：`Text`
- 外部类型：`The`
- 外部类型：`Write`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-7474206f5a` → 本草稿移入 `cases/defect/auto-sqlite-7474206f5a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
