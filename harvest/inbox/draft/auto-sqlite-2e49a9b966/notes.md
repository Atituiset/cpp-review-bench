# auto-sqlite-2e49a9b966

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#c3e073420ff9e35ea80c783e3664e32449b5b703](https://github.com/sqlite/sqlite/commit/c3e073420ff9e35ea80c783e3664e32449b5b703) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-17 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 7 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #c3e073420ff9e35ea80c783e3664e32449b5b703 (https://github.com/sqlite/sqlite/commit/c3e073420ff9e35ea80c783e3664e32449b5b703)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 54；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT c3e073420ff9e35ea80c783e3664e32449b5b703 In test file test1.c, use a static SQLite mutex to protect the hash table instead of a Tcl mutex. A Tcl mutex is a no-op :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -51,10 +51,17 @@
 */
 static struct Test1Global {
   int isInit;
-  Tcl_Mutex m;
   Tcl_HashTable h;
 } g;
 
+static void testEnterHashMutex(){
+  sqlite3_mutex_enter( sqlite3_mutex_alloc(SQLITE_MUTEX_STATIC_APP3) );
+}
+static void testLeaveHashMutex(){
+  sqlite3_mutex_leave( sqlite3_mutex_alloc(SQLITE_MUTEX_STATIC_APP3) );
+}
+
+
 /*
 ** Convert a pointer to a string, and return a pointer to a buffer containing
 ** a copy of that string. The buffer belongs to the global hash table and
@@ -69,7 +76,7 @@ static const char *testPointerToString(void *pPtr){
   if( pPtr==0 ){
     zRet = "0";
   }else{
-    Tcl_MutexLock(&g.m);
+    testEnterHashMutex();
     if( g.isInit==0 ){
       Tcl_InitHashTable(&g.h, TCL_STRING_KEYS);
       g.isInit = 1;
@@ -78,7 +85,7 @@ static const char *testPointerToString(void *pPtr){
     pEntry = Tcl_CreateHashEntry(&g.h, aKey, &dummy);
     Tcl_SetHashValue(pEntry, pPtr);
     zRet = Tcl_GetHashKey(&g.h, pEntry);
-    Tcl_MutexUnlock(&g.m);
+    testLeaveHashMutex();
   }
 
   return zRet;
@@ -92,14 +99,14 @@ static void *testStringToPointer(const char *zStr){
   void *pRet = 0;
 
   if( zStr[0]!='0' ){
-    Tcl_MutexLock(&g.m);
+    testEnterHashMutex();
     if( g.isInit ){
       Tcl_HashEntry *pEntry = Tcl_FindHashEntry(&g.h, zStr);
       if( pEntry ){
         pRet = Tcl_GetHashValue(pEntry);
       }
     }
-    Tcl_MutexUnlock(&g.m);
+    testLeaveHashMutex();
   }
 
   return pRet;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 大写宏：`TCL_STRING_KEYS`
- 外部类型：`Convert`
- 外部类型：`Tcl_HashEntry`
- 外部类型：`Tcl_HashTable`
- 外部类型：`Tcl_Mutex`
- 外部类型：`Test1Global`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-2e49a9b966` → 本草稿移入 `cases/defect/auto-sqlite-2e49a9b966/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
