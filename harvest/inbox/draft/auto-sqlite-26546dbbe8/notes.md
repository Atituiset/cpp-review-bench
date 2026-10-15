# auto-sqlite-26546dbbe8

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#2023a2b37593073de0e789455b6a362ceeecd1d1](https://github.com/sqlite/sqlite/commit/2023a2b37593073de0e789455b6a362ceeecd1d1) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-15 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 74 |
| 编译错误数（gcc syntax-only） | 84（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #2023a2b37593073de0e789455b6a362ceeecd1d1 (https://github.com/sqlite/sqlite/commit/2023a2b37593073de0e789455b6a362ceeecd1d1)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 5525；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 2023a2b37593073de0e789455b6a362ceeecd1d1 Fix json_tree() when it is given a root that ends with an array index so :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -5512,17 +5512,16 @@ static int jsonEachPathLength(JsonEachCursor *p){
   u32 n = p->path.nUsed;
   char *z = p->path.zBuf;
   if( p->iRowid==0 && p->bRecursive && n>=2 ){
-    while( n>1 ){
+    while( ALWAYS(n>1) ){
       n--;
       if( z[n]=='[' || z[n]=='.' ){
-        u32 x, sz = 0;
+        u32 x;
         char cSaved = z[n];
         z[n] = 0;
         assert( p->sParse.eEdit==0 );
         x = jsonLookupStep(&p->sParse, 0, z+1, 0);
         z[n] = cSaved;
-        if( JSON_LOOKUP_ISERROR(x) ) continue;
-        if( x + jsonbPayloadSize(&p->sParse, x, &sz) == p->i ) break;
+        if( !JSON_LOOKUP_ISERROR(x) ) break;
       }
     }
   }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`insert`
- 外部函数：`jsonParseReset`
- 外部函数：`json_insert`
- 外部函数：`json_replace`
- 外部函数：`json_set`
- 外部函数：`jsonbArrayCount`
- 外部函数：`memchr`
- 外部函数：`memcmp`
- 外部函数：`memcpy`
- 外部函数：`memmove`
- 外部函数：`memset`
- 外部函数：`sqlite3DbFree`
- 外部函数：`sqlite3DbRealloc`
- 外部函数：`sqlite3Isdigit`
- 外部函数：`sqlite3RCStrUnref`
- 外部函数：`sqlite3Utf8ReadLimited`
- 外部函数：`sqlite3_strglob`
- 外部函数：`testcase`
- 大写宏：`ALWAYS`
- 大写宏：`BLOB`
- 大写宏：`JSON`
- 大写宏：`JSON5`
- 大写宏：`JSONB`
- 大写宏：`JSONB_`
- 大写宏：`NOTFOUND`
- 大写宏：`NO_TEST`
- 大写宏：`NULL`
- 大写宏：`OOM`
- 大写宏：`PATHERROR`
- 大写宏：`SQL`
- 大写宏：`SQLITE_ASCII`
- 大写宏：`SQLITE_BUG_COMPATIBLE_20250510`
- 大写宏：`SQLITE_EBCDIC`
- 大写宏：`SQLITE_NOINLINE`
- 外部类型：`Allow`
- 外部类型：`Already`
- 外部类型：`An`
- 外部类型：`Because`
- 外部类型：`Begin`
- 外部类型：`But`
- 外部类型：`Bytes`
- 外部类型：`Cannot`
- 外部类型：`Construct`
- 外部类型：`Content`
- 外部类型：`Correct`
- 外部类型：`Delete`
- 外部类型：`Error`
- 外部类型：`First`
- 外部类型：`Header`
- 外部类型：`Index`
- 外部类型：`Insert`
- 外部类型：`JsonParse`
- 外部类型：`Just`
- 外部类型：`Label`
- 外部类型：`Legacy`
- 外部类型：`Lookup`
- 外部类型：`Might`
- 外部类型：`Need`
- 外部类型：`Neither`
- 外部类型：`New`
- 外部类型：`No`
- 外部类型：`Node`
- 外部类型：`Number`
- 外部类型：`One`
- 外部类型：`Os`
- 外部类型：`Overwrite`
- 外部类型：`Populate`
- 外部类型：`Quirks`
- 外部类型：`SQLite`
- 外部类型：`Simpliest`
- 外部类型：`Size`
- 外部类型：`Tail`
- 外部类型：`Text`
- 外部类型：`That`
- 外部类型：`The`
- 外部类型：`Total`
- 外部类型：`True`
- 外部类型：`Use`
- 外部类型：`Write`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-26546dbbe8` → 本草稿移入 `cases/defect/auto-sqlite-26546dbbe8/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
