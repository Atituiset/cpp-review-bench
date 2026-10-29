# auto-sqlite-e0cc0b91ca

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#1a434126ab2bd95fab2c65c0d6d458d2674b5540](https://github.com/sqlite/sqlite/commit/1a434126ab2bd95fab2c65c0d6d458d2674b5540) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-29 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 4 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1a434126ab2bd95fab2c65c0d6d458d2674b5540 (https://github.com/sqlite/sqlite/commit/1a434126ab2bd95fab2c65c0d6d458d2674b5540)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 1a434126ab2bd95fab2c65c0d6d458d2674b5540 Add commented-out code that *would* support +NaN and -NaN literals :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1874,6 +1874,7 @@ static int jsonTranslateTextToBlob(JsonParse *pParse, u32 i){
         }
       }else{
         if( !sqlite3Isdigit(z[i+1]) ){
+          assert( z[i]=='-' || z[i]=='+' );
           /* JSON5 allows for "+Infinity" and "-Infinity" using exactly
           ** that case.  SQLite also allows these in any case and it allows
           ** "+inf" and "-inf". */
@@ -1893,6 +1894,18 @@ static int jsonTranslateTextToBlob(JsonParse *pParse, u32 i){
             t |= 0x01;
             goto parse_number_2;
           }
+#if 0
+          /* Full JSON5 compliance requires support for +NaN and -NaN, even
+          ** though the extra prefix is not used for anything.  Enable this
+          ** block of code to get SQLite to do the same thing.  It never
+          ** has in an official release, and the only accomplishment of
+          ** enabling it would be to take up a few bytes more code space. */
+          if( sqlite3StrNICmp(&z[i+1],"NaN",3)==0 ){
+            pParse->hasNonstd = 1;
+            jsonBlobAppendOneByte(pParse, JSONB_NULL);
+            return i+4;
+          }
+#endif
           pParse->iErr = i;
           return -1;
         }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`sqlite3Isdigit`
- 大写宏：`JSON5`
- 外部类型：`Infinity`
- 外部类型：`SQLite`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-e0cc0b91ca` → 本草稿移入 `cases/defect/auto-sqlite-e0cc0b91ca/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
