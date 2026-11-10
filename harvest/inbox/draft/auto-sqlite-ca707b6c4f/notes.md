# auto-sqlite-ca707b6c4f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#e0a6d50fa4eef0de4079902df9325a75cc254318](https://github.com/sqlite/sqlite/commit/e0a6d50fa4eef0de4079902df9325a75cc254318) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-10 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 6 |
| 编译错误数（gcc syntax-only） | 6（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #e0a6d50fa4eef0de4079902df9325a75cc254318 (https://github.com/sqlite/sqlite/commit/e0a6d50fa4eef0de4079902df9325a75cc254318)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 787；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT e0a6d50fa4eef0de4079902df9325a75cc254318 Enhance the TEXT&rarr;REAL converter so that it works even for ridiculous :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -784,7 +784,7 @@ static void sqlite3Fp2Convert10(u64 m, int e, int n, u64 *pD, int *pP){
 ** The (current) algorithm is adapted from the work of Ross Cox at
 ** https://github.com/rsc/fpfmt
 */
-static double sqlite3Fp10Convert2(u64 d, int p){
+static double sqlite3Fp10Convert2(u64 d, i64 p){
   int b, lp, e, adj, s;
   u32 pwr10l, mid1;
   u64 pwr10h, x, hi, lo, sticky, u, m;
@@ -882,7 +882,7 @@ int sqlite3AtoF(const char *zIn, double *pResult){
   const unsigned char *z = (const unsigned char*)zIn;
   int neg = 0;       /* True for a negative value */
   u64 s = 0;         /* mantissa */
-  int d = 0;         /* Value is s * pow(10,d) */
+  i64 d = 0;         /* Value is s * pow(10,d) */
   int mState = 0;    /* 1: digit seen 2: fp 4: hard-zero */
   unsigned v;        /* Value of a single digit */
 
@@ -960,14 +960,14 @@ int sqlite3AtoF(const char *zIn, double *pResult){
     }
     /* copy digits to exponent */
     if( (v = (unsigned)z[0] - '0')<10 ){
-      int exp = v;
+      u64 exp = v;
       z++;
       mState |= 2;
       while( (v = (unsigned)z[0] - '0')<10 ){
-        exp = exp<10000 ? (exp*10 + v) : 10000;
+        exp = exp*10 + v + (exp & (0xffULL<<32));
         z++;
       }
-      d += esign*exp;
+      d += esign*(exp & ((1ULL<<48)-1));
     }else{
       z--;  /* Leave z[0] at 'e' or '+' or '-',
             ** so that the return is 0 or -1 */
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`pow`
- 外部类型：`Cox`
- 外部类型：`Leave`
- 外部类型：`Ross`
- 外部类型：`True`
- 外部类型：`Value`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-ca707b6c4f` → 本草稿移入 `cases/defect/auto-sqlite-ca707b6c4f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
