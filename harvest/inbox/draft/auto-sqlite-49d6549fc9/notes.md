# auto-sqlite-49d6549fc9

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#47b4187a62d3fabc64cc9d779546eba9c29e233b](https://github.com/sqlite/sqlite/commit/47b4187a62d3fabc64cc9d779546eba9c29e233b) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-31 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 7 |
| 编译错误数（gcc syntax-only） | 33（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #47b4187a62d3fabc64cc9d779546eba9c29e233b (https://github.com/sqlite/sqlite/commit/47b4187a62d3fabc64cc9d779546eba9c29e233b)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 694；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 47b4187a62d3fabc64cc9d779546eba9c29e233b Fix the REGEXP extension so that "|" has higher precedence than "^". :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -691,11 +691,7 @@ static const char *re_compile(
     re_free(pRe);
     return zErr;
   }
-  if( zIn[0]=='^' ){
-    zIn++;
-  }else{
-    re_append(pRe, RE_OP_ANYSTAR, 0);
-  }
+  re_append(pRe, RE_OP_ANYSTAR, 0);
   pRe->sIn.z = (unsigned char*)zIn;
   pRe->sIn.i = 0;
   pRe->sIn.mx = (int)strlen(zIn);
@@ -712,6 +708,19 @@ static const char *re_compile(
     return "unrecognized character";
   }
 
+  /* If the regex begins with "^" and has no "|" at the top level, then
+  ** the whole regex is anchored at the start of the input and the
+  ** initial ".*" is not needed.  Convert it into a no-op so that
+  ** re_match() can stop as soon as the start of the input fails to
+  ** match.  A "|" at the top level would have caused a FORK to be
+  ** inserted at aOp[1], so aOp[1]==RE_OP_ATSTART is sufficient.  Do
+  ** not remove the initial "^" itself, as there might be jumps back
+  ** to it, as in "^+". */
+  if( pRe->aOp[1]==RE_OP_ATSTART ){
+    pRe->aOp[0] = RE_OP_GOTO;
+    pRe->aArg[0] = 1;
+  }
+
   /* The following is a performance optimization.  If the regex begins with
   ** ".*" (if the input regex lacks an initial "^") and afterwards there are
   ** one or more matching characters, enter those matching characters into
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`re_free`
- 外部函数：`sqlite3_realloc64`
- 外部函数：`strlen`
- 大写宏：`REGEXP`
- 外部类型：`If`
- 外部类型：`ReCompiled`
- 外部类型：`Special`
- 外部类型：`The`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-49d6549fc9` → 本草稿移入 `cases/defect/auto-sqlite-49d6549fc9/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
