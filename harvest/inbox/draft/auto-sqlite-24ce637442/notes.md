# auto-sqlite-24ce637442

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#1e4e45df2465b56feeccc75f413be502768f95a9](https://github.com/sqlite/sqlite/commit/1e4e45df2465b56feeccc75f413be502768f95a9) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-29 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 24 |
| 编译错误数（gcc syntax-only） | 11（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1e4e45df2465b56feeccc75f413be502768f95a9 (https://github.com/sqlite/sqlite/commit/1e4e45df2465b56feeccc75f413be502768f95a9)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 299；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 1e4e45df2465b56feeccc75f413be502768f95a9 Fixes to the LIKE optimization to catch new corner cases where it cannot :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -161,6 +161,43 @@ static u16 operatorMask(int op){
   return c;
 }
 
+#ifndef SQLITE_OMIT_LIKE_OPTIMIZATION
+/*
+** The input is an N-byte GLOB or LIKE pattern prefix
+** with the following guarantees:
+**
+**   (1)  It contains no wildcards.
+**   (2)  z[N]==0
+**   (3)  N>0
+**   (4)  bHasSpace is true if and only if the original complete LIKE/GLOB
+**        pattern contained wildcards and/or escapes.  (It is usually true,
+**        therefore).  bHasSpace also means that there is at least one byte
+**        of overrun space at z[N+1] that this routine may use.
+**
+** This routine checks to see if the input will be a prefix to
+** any number (integer or floating-point) that SQLite might generate
+** from a MEM_Int or MEM_Real, including "Inf" and "-Inf".
+**
+** Return true if the input is a possible numeric prefix and
+** false if the input cannot possibly be a numeric prefix.
+*/
+static SQLITE_NOINLINE int isNumericPrefix(char *z, int N, int bHasSpace){
+  int x;
+  double rDummy;
+  assert( N>0 );
+  if( sqlite3AtoF(z, &rDummy)>0 ) return 1; /* Boundary is itself a number */
+  if( bHasSpace ){
+    z[N] = '0';
+    z[N+1] = 0;
+    x = sqlite3AtoF(z, &rDummy);            /* Prefix like "1.0e" or "-" */
+    z[N] = 0;
+    if( x>0 ) return 1;
+  }
+  if( z[0]=='-' ){ z++; N--; }
+  if( N>0 && N<=3 && sqlite3StrNICmp(z,"inf",N)==0 ) return 1;
+  return 0;
+}
+#endif /* SQLITE_OMIT_LIKE_OPTIMIZATION */
 
 #ifndef SQLITE_OMIT_LIKE_OPTIMIZATION
 /*
@@ -216,7 +253,7 @@ static int isLikeOrGlob(
     assert( pRight->op==TK_VARIABLE || pRight->op==TK_REGISTER );
   }else if( op==TK_STRING ){
     assert( !ExprHasProperty(pRight, EP_IntValue) );
-     z = (u8*)pRight->u.zToken;
+    z = (u8*)pRight->u.zToken;
   }
   if( z ){
     /* Count the number of prefix bytes prior to the first wildcard,
@@ -243,12 +280,12 @@ static int isLikeOrGlob(
       }
     }
 
-    /* The optimization is possible only if (1) the pattern does not begin
-    ** with a wildcard and if (2) the non-wildcard prefix does not end with
-    ** an (illegal 0xff) character, or (3) the pattern does not consist of
-    ** a single escape character. The second condition is necessary so
+    /* The optimization is possible only if either (1a) the pattern does not
+    ** begin with a wildcard and if (1b) the non-wildcard prefix does not end
+    ** with an (illegal) 0xff character, or (2) the pattern does not consist of
+    ** a single escape character. The (1b) condition is necessary so
     ** that we can increment the prefix key to find an upper bound for the
-    ** range search. The third is because the caller assumes that the pattern
+    ** range search. The (2) is because the caller assumes that the pattern
     ** consists of at least one character after all escapes have been
     ** removed.  */
     if( (cnt>1 || (cnt>0 && z[0]!=wc[3])) && ALWAYS(255!=(u8)z[cnt-1]) ){
@@ -274,8 +311,11 @@ static int isLikeOrGlob(
 
         /* If the LHS is not an ordinary column with TEXT affinity, then the
         ** pattern prefix boundaries (both the start and end boundaries) must
-        ** not look like a number.  Otherwise the pattern might be treated as
-        ** a number, which will invalidate the LIKE optimization.
+        ** not look like a number or the prefix of a number.  Otherwise the
+        ** pattern might might match against a number, which means this
+        ** optimization will not work.  If either the start or end boundary
+        ** strings has any possibility of matching against a text rendering
+        ** of a MEM_Int or MEM_Real, then we must return 0.
         **
         ** Getting this right has been a persistent source of bugs in the
         ** LIKE optimization.  See, for example:
@@ -284,27 +324,24 @@ static int isLikeOrGlob(
         **    2019-06-10 https://sqlite.org/src/info/fd76310a5e843e07
         **    2019-06-14 https://sqlite.org/src/info/ce8717f0885af975
         **    2019-09-03 https://sqlite.org/src/info/0f
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`an`
- 外部函数：`assert`
- 外部函数：`or`
- 外部函数：`sqlite3AtoF`
- 外部函数：`sqlite3ExprAffinity`
- 外部函数：`sqlite3ExprDelete`
- 外部函数：`sqlite3ValueFree`
- 大写宏：`ALWAYS`
- 大写宏：`LHS`
- 大写宏：`LIKE`
- 大写宏：`SQLITE_AFF_TEXT`
- 大写宏：`SQLITE_OMIT_LIKE_OPTIMIZATION`
- 大写宏：`TEXT`
- 大写宏：`TK_COLUMN`
- 大写宏：`TK_REGISTER`
- 大写宏：`TK_STRING`
- 大写宏：`TK_VARIABLE`
- 外部类型：`Count`
- 外部类型：`EP_IntValue`
- 外部类型：`Getting`
- 外部类型：`If`
- 外部类型：`Might`
- 外部类型：`Otherwise`
- 外部类型：`See`
- 外部类型：`The`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-24ce637442` → 本草稿移入 `cases/defect/auto-sqlite-24ce637442/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
