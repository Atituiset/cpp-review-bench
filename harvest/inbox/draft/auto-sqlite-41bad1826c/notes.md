# auto-sqlite-41bad1826c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#1778ff4dabcc90645f573e11aee688cbe406409d](https://github.com/sqlite/sqlite/commit/1778ff4dabcc90645f573e11aee688cbe406409d) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-24 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 21 |
| 编译错误数（gcc syntax-only） | 22（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1778ff4dabcc90645f573e11aee688cbe406409d (https://github.com/sqlite/sqlite/commit/1778ff4dabcc90645f573e11aee688cbe406409d)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 1391；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 1778ff4dabcc90645f573e11aee688cbe406409d Merge BOM handling fixes. :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1307,11 +1307,13 @@ int sqlite3VdbeMemSetStr(
   i64 nByte = n;      /* New value for pMem->n */
   int iLimit;         /* Maximum allowed string or blob size */
   u16 flags;          /* New value for pMem->flags */
+  u8 xenc;            /* Encoding without the ENC_KEEPBOM bit */
 
   assert( pMem!=0 );
   assert( pMem->db==0 || sqlite3_mutex_held(pMem->db->mutex) );
   assert( !sqlite3VdbeMemIsRowSet(pMem) );
-  assert( enc!=0 || n>=0 );
+  xenc = enc & ENC_MASK;
+  assert( xenc!=0 || n>=0 );
 
   /* If z is a NULL pointer, set pMem to contain an SQL NULL. */
   if( !z ){
@@ -1325,16 +1327,16 @@ int sqlite3VdbeMemSetStr(
     iLimit = SQLITE_MAX_LENGTH;
   }
   if( nByte<0 ){
-    assert( enc!=0 );
-    if( enc==SQLITE_UTF8 ){
+    assert( xenc!=0 );
+    if( xenc==SQLITE_UTF8 ){
       nByte = strlen(z);
     }else{
       for(nByte=0; nByte<=iLimit && (z[nByte] | z[nByte+1]); nByte+=2){}
     }
     flags= MEM_Str|MEM_Term;
-  }else if( enc==0 ){
+  }else if( xenc==0 ){
     flags = MEM_Blob;
-    enc = SQLITE_UTF8;
+    xenc = SQLITE_UTF8;
   }else{
     flags = MEM_Str;
   }
@@ -1357,7 +1359,7 @@ int sqlite3VdbeMemSetStr(
   if( xDel==SQLITE_TRANSIENT ){
     i64 nAlloc = nByte;
     if( flags&MEM_Term ){
-      nAlloc += (enc==SQLITE_UTF8?1:2);
+      nAlloc += (xenc==SQLITE_UTF8?1:2);
     }
     testcase( nAlloc==0 );
     testcase( nAlloc==31 );
@@ -1381,14 +1383,13 @@ int sqlite3VdbeMemSetStr(
 
   pMem->n = (int)(nByte & 0x7fffffff);
   pMem->flags = flags;
-  pMem->enc = enc;
+  pMem->enc = xenc;
 
 #ifndef SQLITE_OMIT_UTF16
-  /* tag-20261016-1:  To retain the BOM, use enc==SQLITE_UTF8, even
-  ** for UTF-16 text, and then set pMem->enc correctly after this
-  ** function returns.  Note, however, that n must be non-negative
-  ** for this trick to work. */
-  if( enc>SQLITE_UTF8 && sqlite3VdbeMemHandleBom(pMem) ){
+  if( xenc>SQLITE_UTF8
+   && (enc & ENC_KEEPBOM)==0
+   && sqlite3VdbeMemHandleBom(pMem)
+  ){
     return SQLITE_NOMEM_BKPT;
   }
 #endif
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`sqlite3VdbeMemHandleBom`
- 外部函数：`sqlite3_mutex_held`
- 外部函数：`strlen`
- 外部函数：`testcase`
- 大写宏：`BOM`
- 大写宏：`NULL`
- 大写宏：`SQL`
- 大写宏：`SQLITE_MAX_LENGTH`
- 大写宏：`SQLITE_NOMEM_BKPT`
- 大写宏：`SQLITE_OMIT_UTF16`
- 大写宏：`SQLITE_TRANSIENT`
- 大写宏：`SQLITE_UTF8`
- 大写宏：`UTF`
- 外部类型：`If`
- 外部类型：`MEM_Blob`
- 外部类型：`MEM_Dyn`
- 外部类型：`MEM_Str`
- 外部类型：`MEM_Term`
- 外部类型：`Maximum`
- 外部类型：`Mem`
- 外部类型：`New`
- 外部类型：`Note`
- 外部类型：`To`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-41bad1826c` → 本草稿移入 `cases/defect/auto-sqlite-41bad1826c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
