# auto-sqlite-6dd8534d7e

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#b8f3f5e1d5d475ba07945ec86e3255e5efdc0135](https://github.com/sqlite/sqlite/commit/b8f3f5e1d5d475ba07945ec86e3255e5efdc0135) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-18 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 19 |
| 编译错误数（gcc syntax-only） | 16（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #b8f3f5e1d5d475ba07945ec86e3255e5efdc0135 (https://github.com/sqlite/sqlite/commit/b8f3f5e1d5d475ba07945ec86e3255e5efdc0135)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1085；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT b8f3f5e1d5d475ba07945ec86e3255e5efdc0135 Make it an error to configure secure-delete mode on a contentless_delete=1 fts5 table. Prior to this it was not an error :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -927,14 +927,15 @@ int sqlite3Fts5ConfigSetValue(
   int *pbBadkey
 ){
   int rc = SQLITE_OK;
+  int bBadkey = 0;
 
   if( 0==sqlite3_stricmp(zKey, "pgsz") ){
     int pgsz = 0;
     if( SQLITE_INTEGER==sqlite3_value_numeric_type(pVal) ){
       pgsz = sqlite3_value_int(pVal);
     }
     if( pgsz<32 || pgsz>FTS5_MAX_PAGE_SIZE ){
-      *pbBadkey = 1;
+      bBadkey = 1;
     }else{
       pConfig->pgsz = pgsz;
     }
@@ -946,7 +947,7 @@ int sqlite3Fts5ConfigSetValue(
       nHashSize = sqlite3_value_int(pVal);
     }
     if( nHashSize<=0 ){
-      *pbBadkey = 1;
+      bBadkey = 1;
     }else{
       pConfig->nHashSize = nHashSize;
     }
@@ -958,7 +959,7 @@ int sqlite3Fts5ConfigSetValue(
       nAutomerge = sqlite3_value_int(pVal);
     }
     if( nAutomerge<0 || nAutomerge>64 ){
-      *pbBadkey = 1;
+      bBadkey = 1;
     }else{
       if( nAutomerge==1 ) nAutomerge = FTS5_DEFAULT_AUTOMERGE;
       pConfig->nAutomerge = nAutomerge;
@@ -971,7 +972,7 @@ int sqlite3Fts5ConfigSetValue(
       nUsermerge = sqlite3_value_int(pVal);
     }
     if( nUsermerge<2 || nUsermerge>16 ){
-      *pbBadkey = 1;
+      bBadkey = 1;
     }else{
       pConfig->nUsermerge = nUsermerge;
     }
@@ -983,7 +984,7 @@ int sqlite3Fts5ConfigSetValue(
       nCrisisMerge = sqlite3_value_int(pVal);
     }
     if( nCrisisMerge<0 ){
-      *pbBadkey = 1;
+      bBadkey = 1;
     }else{
       if( nCrisisMerge<=1 ) nCrisisMerge = FTS5_DEFAULT_CRISISMERGE;
       if( nCrisisMerge>=FTS5_MAX_SEGMENT ) nCrisisMerge = FTS5_MAX_SEGMENT-1;
@@ -996,7 +997,7 @@ int sqlite3Fts5ConfigSetValue(
     if( SQLITE_INTEGER==sqlite3_value_numeric_type(pVal) ){
       nVal = sqlite3_value_int(pVal);
     }else{
-      *pbBadkey = 1;
+      bBadkey = 1;
     }
     if( nVal<0 ) nVal = FTS5_DEFAULT_DELETE_AUTOMERGE;
     if( nVal>100 ) nVal = 0;
@@ -1015,7 +1016,7 @@ int sqlite3Fts5ConfigSetValue(
       pConfig->zRankArgs = zRankArgs;
     }else if( rc==SQLITE_ERROR ){
       rc = SQLITE_OK;
-      *pbBadkey = 1;
+      bBadkey = 1;
     }
   }
 
@@ -1025,7 +1026,13 @@ int sqlite3Fts5ConfigSetValue(
       bVal = sqlite3_value_int(pVal);
     }
     if( bVal<0 ){
-      *pbBadkey = 1;
+      bBadkey = 1;
+    }else if( bVal>0 && pConfig->bContentlessDelete && pbBadkey ){
+      /* Secure-delete may not be enabled on a contentless_delete=1 table. */
+      sqlite3Fts5ConfigErrmsg(pConfig,
+          "'secure-delete' may not be used with a contentless_delete=1 table"
+      );
+      rc = SQLITE_ERROR;
     }else{
       pConfig->bSecureDelete = (bVal ? 1 : 0);
     }
@@ -1037,14 +1044,16 @@ int sqlite3Fts5ConfigSetValue(
       bVal = sqlite3_value_int(pVal);
     }
     if( bVal<0 ){
-      *pbBadkey = 1;
+      bBadkey = 1;
     }else{
       pConfig->bPrefixInsttoken = (bVal ? 1 : 0);
     }
 
   }else{
-    *pbBadkey = 1;
+    bBadkey = 1;
   }
+
+  if( pbBadkey ) *pbBadkey = bBadkey;
   return rc;
 }
 
@@ -1082,8 +1091,7 @@ int sqlite3Fts5ConfigLoad(Fts5Config *pConfig, int iCookie){
       if( 0==sqlite3_stricmp(zK, "version") ){
         iVersion = sqlite3_value_int(pVal);
       }else{
-        int bDummy = 0;
-        sqlite3Fts5ConfigSetValue(pConfig, zK, pVal, &bDummy);
+        sqlite3Fts5ConfigSetValue(pConfig, zK, pVal, 0);
       }
     }
     rc = sqlite3_finalize(p);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`memcpy`
- 外部函数：`sqlite3Fts5IsBareword`
- 外部函数：`sqlite3Fts5MallocZero`
- 外部函数：`sqlite3_finalize`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_stricmp`
- 外部函数：`sqlite3_strnicmp`
- 外部函数：`sqlite3_value_int`
- 外部函数：`sqlite3_value_numeric_type`
- 外部函数：`sqlite3_value_text`
- 大写宏：`FTS5_MAX_SEGMENT`
- 大写宏：`OUT`
- 大写宏：`SQLITE_ERROR`
- 大写宏：`SQLITE_INTEGER`
- 大写宏：`SQLITE_OK`
- 外部类型：`At`
- 外部类型：`Fts5Config`
- 外部类型：`Input`
- 外部类型：`Or`
- 外部类型：`Rank`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-6dd8534d7e` → 本草稿移入 `cases/defect/auto-sqlite-6dd8534d7e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
