# auto-sqlite-dd6ffe587f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#e9183c57f2f8544b35f82b163a140af4bd35d1cc](https://github.com/sqlite/sqlite/commit/e9183c57f2f8544b35f82b163a140af4bd35d1cc) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-16 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 9 |
| 编译错误数（gcc syntax-only） | 48（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #e9183c57f2f8544b35f82b163a140af4bd35d1cc (https://github.com/sqlite/sqlite/commit/e9183c57f2f8544b35f82b163a140af4bd35d1cc)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 313；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT e9183c57f2f8544b35f82b163a140af4bd35d1cc Merge the 24:00:00 fix from branch-3.54 into trunk :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -302,31 +302,34 @@ static void computeJD(DateTime *p){
   }
 }
 
+/*
+** Return the number of dates in a specific month.
+*/
+static int daysOfMonth(int Y, int M){
+  if( (1<<M) & 0x15aa ){
+    return 31;
+  }else if( M!=2 ){
+    return 30;
+  }else if( Y%4==0 && (Y%100!=0 || Y%400==0) ){
+    return 29;
+  }else{
+    return 28;
+  }
+}
+
 /*
 ** Given the YYYY-MM-DD information current in p, determine if there
 ** is day-of-month overflow and set nFloor to the number of days that
 ** would need to be subtracted from the date in order to bring the
 ** date back to the end of the month.
 */
 static void computeFloor(DateTime *p){
+  int mxD;         /* Maximum day number for the current month */
   assert( p->validYMD || p->isError );
-  assert( p->D>=0 && p->D<=31 );
+  assert( p->D>=0 && p->D<=32 );
   assert( p->M>=0 && p->M<=12 );
-  if( p->validHMS && p->h==24 ){
-    p->h = 0;
-    p->D++;
-  }
-  if( p->D<=28 ){
-    p->nFloor = 0;
-  }else if( (1<<p->M) & 0x15aa ){
-    p->nFloor = 0;
-  }else if( p->M!=2 ){
-    p->nFloor = (p->D==31);
-  }else if( p->Y%4!=0 || (p->Y%100==0 && p->Y%400!=0) ){
-    p->nFloor = p->D - 28;
-  }else{
-    p->nFloor = p->D - 29;
-  }
+  mxD = daysOfMonth(p->Y, p->M);
+  p->nFloor = p->D>mxD ? p->D - mxD : 0;
 }
 
 /*
@@ -356,7 +359,19 @@ static int parseYyyyMmDd(const char *zDate, DateTime *p){
   zDate += 10;
   while( sqlite3Isspace(*zDate) || 'T'==*(u8*)zDate ){ zDate++; }
   if( parseHhMmSs(zDate, p)==0 ){
-    /* We got the time */
+    if( p->h==24 ){
+      int mxD = daysOfMonth(Y,M);
+      p->h = 0;
+      D++;
+      if( D>mxD ){
+        D -= mxD;
+        M++;
+        if( M>12 ){
+          M = 1;
+          Y++;
+        }
+      }
+    }
   }else if( *zDate==0 ){
     p->validHMS = 0;
   }else{
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`sqlite3Isdigit`
- 外部函数：`sqlite3Isspace`
- 外部函数：`va_arg`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 大写宏：`YYYY`
- 外部类型：`DateTime`
- 外部类型：`Forum`
- 外部类型：`Given`
- 外部类型：`The`
- 外部类型：`Truncate`
- 外部类型：`We`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-dd6ffe587f` → 本草稿移入 `cases/defect/auto-sqlite-dd6ffe587f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
