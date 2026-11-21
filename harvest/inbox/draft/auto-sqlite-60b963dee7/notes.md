# auto-sqlite-60b963dee7

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#7dd00550bf588547c66f1084f531d91b28805910](https://github.com/sqlite/sqlite/commit/7dd00550bf588547c66f1084f531d91b28805910) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-21 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 3 |
| 编译错误数（gcc syntax-only） | 9（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #7dd00550bf588547c66f1084f531d91b28805910 (https://github.com/sqlite/sqlite/commit/7dd00550bf588547c66f1084f531d91b28805910)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 994；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 7dd00550bf588547c66f1084f531d91b28805910 Disable the SQLITE_DBCONFIG_FP_DIGITS settings, as it causes far more problems :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -989,11 +989,10 @@ int sqlite3_db_config(sqlite3 *db, int op, ...){
       break;
     }
     case SQLITE_DBCONFIG_FP_DIGITS: {
-      int nIn = va_arg(ap, int);
-      int *pOut = va_arg(ap, int*);
-      if( nIn>3 && nIn<24 ) db->nFpDigit = (u8)nIn;
-      if( pOut ) *pOut = db->nFpDigit;
-      rc = SQLITE_OK;
+      /* Deliberate no-op as of 2026-11-20, as changing the float->text
+      ** algorithm can cause stale indexes in schemas that misuse
+      ** floating point values.  SQLITE_TESTCTRL_FP_DIGITS is provided
+      ** as an alternative, for testing purposes only. */
       break;
     }
     default: {
@@ -4894,6 +4893,24 @@ int sqlite3_test_control(int op, ...){
 #endif
       break;
     }
+
+    /* sqlite3_test_control(SQLITE_TESTCTRL_FP_DIGITS,db,nIn,pnOut)
+    **
+    ** Change the number of digits of precision used for float->text
+    ** conversions for database connection db to nIn (range 4..23).
+    ** If nIn is out of range, make no changes.
+    ** If pnOut is not NULL, write value after changes into *pnOut.
+    **
+    ** This replaces SQLITE_DBCONFIG_FP_DIGITS as of 2026-11-20.
+    */
+    case SQLITE_TESTCTRL_FP_DIGITS: {
+      sqlite3 *db = va_arg(ap, sqlite3*);
+      int nIn = va_arg(ap, int);
+      int *pOut = va_arg(ap, int*);
+      if( db && nIn>3 && nIn<24 ) db->nFpDigit = (u8)nIn;
+      if( pOut ) *pOut = db ? db->nFpDigit : 0;
+      break;
+    }
   }
   va_end(ap);
 #endif /* SQLITE_UNTESTABLE */
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`va_arg`
- 外部函数：`va_end`
- 大写宏：`SQLITE_DBCONFIG_FP_DIGITS`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_UNTESTABLE`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-60b963dee7` → 本草稿移入 `cases/defect/auto-sqlite-60b963dee7/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
