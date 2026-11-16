# auto-sqlite-ea40640dfb

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#8a24548dfbe71bc2714079e7549d1814002f5c44](https://github.com/sqlite/sqlite/commit/8a24548dfbe71bc2714079e7549d1814002f5c44) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-16 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 4 |
| 编译错误数（gcc syntax-only） | 4（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #8a24548dfbe71bc2714079e7549d1814002f5c44 (https://github.com/sqlite/sqlite/commit/8a24548dfbe71bc2714079e7549d1814002f5c44)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 8a24548dfbe71bc2714079e7549d1814002f5c44 Make most tunable parameters in the query planner adjustable using :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -296,9 +296,6 @@ SQLITE_WSD struct Sqlite3Config sqlite3Config = {
    0x7ffffffe,                /* iOnceResetThreshold */
    SQLITE_DEFAULT_SORTERREF_SIZE,   /* szSorterRef */
    0,                         /* iPrngSeed */
-#ifdef SQLITE_DEBUG
-   {0,0,0,0,0,0},             /* aTune */
-#endif
 };
 
 /*
@@ -338,6 +335,49 @@ unsigned int sqlite3CoverageCounter;
 sqlite3_uint64 sqlite3NProfileCnt = 0;
 #endif
 
+#if defined(SQLITE_DEBUG) || defined(SQLITE_ENABLE_TUNING)
+/* Query planner tuning parameters.  Development and testing use
+** only.
+**
+** This array must be kept in synchronization with the various
+** "Tuning(D,ID)" macros in the code (mostly in where.c and select.c).
+** Updates in the code must be replicated in this array.  The
+** tool/tuningverify.tcl script runs at build time to verify that
+** this array is in alignment with all Tuning() macros.
+*/
+int sqlite3Tuning[28] = {
+  27,  /* Number of tuning parameters */
+  30,  /*  1 */
+  20,  /*  2 */
+  20,  /*  3 */
+  46,  /*  4 */
+  10,  /*  5 */
+  10,  /*  6 */
+  10,  /*  7 */
+  42,  /*  8 */
+   5,  /*  9 */
+  16,  /* 10 */
+   3,  /* 11 */
+  28,  /* 12 */
+  25,  /* 13 */
+  43,  /* 14 */
+   1,  /* 15 */
+  10,  /* 16 */
+   6,  /* 17 */
+  10,  /* 18 */
+   5,  /* 19 */
+   3,  /* 20 */
+   2,  /* 21 */
+  33,  /* 22 */
+  39,  /* 23 */
+  30,  /* 24 */
+  18,  /* 25 */
+  12,  /* 26 */
+  48,  /* 27 */
+};
+#endif /* SQLITE_DEBUG || SQLITE_ENABLE_TUNING */
+
+
 /*
 ** The value of the "pending" byte must be 0x40000000 (1 byte past the
 ** 1-gibabyte boundary) in a compatible database.  SQLite never uses
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 大写宏：`SQLITE_DEBUG`
- 大写宏：`SQLITE_DEFAULT_SORTERREF_SIZE`
- 外部类型：`SQLite`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-ea40640dfb` → 本草稿移入 `cases/defect/auto-sqlite-ea40640dfb/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
