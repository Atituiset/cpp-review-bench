# auto-sqlite-101b30da67

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#99d15842d0a5357e6a33be2456f9c969da43fac8](https://github.com/sqlite/sqlite/commit/99d15842d0a5357e6a33be2456f9c969da43fac8) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-11 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 16 |
| 编译错误数（gcc syntax-only） | 17（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #99d15842d0a5357e6a33be2456f9c969da43fac8 (https://github.com/sqlite/sqlite/commit/99d15842d0a5357e6a33be2456f9c969da43fac8)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 207；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 99d15842d0a5357e6a33be2456f9c969da43fac8 Do not allow timezones on a HH:MM:SS offset modifier. :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -204,7 +204,7 @@ static int parseTimezone(const char *zDate, DateTime *p){
 **
 ** Return 1 if there is a parsing error and 0 on success.
 */
-static int parseHhMmSs(const char *zDate, DateTime *p){
+static int parseHhMmSs(const char *zDate, DateTime *p, int bAllowTZ){
   int h, m, s;
   double ms = 0.0;
   if( getDigits(zDate, "20c:20e", &h, &m)!=2 ){
@@ -240,6 +240,7 @@ static int parseHhMmSs(const char *zDate, DateTime *p){
   p->h = h;
   p->m = m;
   p->s = s + ms;
+  if( !bAllowTZ && zDate[0] ) return 1;
   if( parseTimezone(zDate, p) ) return 1;
   return 0;
 }
@@ -359,7 +360,7 @@ static int parseYyyyMmDd(const char *zDate, DateTime *p){
   if( neg ) Y = -Y;
   zDate += 10;
   while( sqlite3Isspace(*zDate) || 'T'==*(u8*)zDate ){ zDate++; }
-  if( parseHhMmSs(zDate, p)==0 ){
+  if( parseHhMmSs(zDate, p, 1)==0 ){
     if( p->h==24 ){
       int mxD = daysOfMonth(Y,M);
       p->h = 0;
@@ -450,7 +451,7 @@ static int parseDateOrTime(
   double r;
   if( parseYyyyMmDd(zDate,p)==0 ){
     return 0;
-  }else if( parseHhMmSs(zDate, p)==0 ){
+  }else if( parseHhMmSs(zDate, p, 1)==0 ){
     return 0;
   }else if( sqlite3StrICmp(zDate,"now")==0 && sqlite3NotPureFunc(context) ){
     return setDateTimeToCurrent(context, p);
@@ -1106,19 +1107,21 @@ static int parseModifier(
 
         DateTime tx;
         sqlite3_int64 day;
-        int nDay;              /* 1 for a shift of exactly 24:00 */
+        int is24hr;            /* 1 for a shift of exactly 24:00 */
         int bTimeOnly = z2==z; /* This is not the HH:MM tail of a
                                ** +/-YYYY-MM-DD HH:MM shift, whose own
                                ** nFloor must survive (e0fe679e7c) */
         if( !sqlite3Isdigit(*z2) ) z2++;
         memset(&tx, 0, sizeof(tx));
-        if( parseHhMmSs(z2, &tx) ) break;
-        nDay = tx.h==24 && tx.tz==0;
+        if( parseHhMmSs(z2, &tx, 0) ){
+          break;  /* HH:MM:SS.FFF failed to parse */
+        }
+        is24hr = tx.h==24;
         computeJD(&tx);
         tx.iJD -= 43200000;
         day = tx.iJD/86400000;
         tx.iJD -= day*86400000;
-        tx.iJD += nDay*(sqlite3_int64)86400000;
+        if( is24hr ) tx.iJD += (sqlite3_int64)86400000;
         if( z0=='-' ) tx.iJD = -tx.iJD;
         computeJD(p);
         clearYMD_HMS_TZ(p);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`memset`
- 外部函数：`sqlite3Isdigit`
- 外部函数：`sqlite3Isspace`
- 外部函数：`sqlite3NotPureFunc`
- 外部函数：`sqlite3StmtCurrentTime`
- 外部函数：`sqlite3StrICmp`
- 外部函数：`survive`
- 外部函数：`va_arg`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 大写宏：`YMD`
- 大写宏：`YYYY`
- 外部类型：`DateTime`
- 外部类型：`Forum`
- 外部类型：`If`
- 外部类型：`Jan`
- 外部类型：`Maximum`
- 外部类型：`Return`
- 外部类型：`The`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-101b30da67` → 本草稿移入 `cases/defect/auto-sqlite-101b30da67/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
