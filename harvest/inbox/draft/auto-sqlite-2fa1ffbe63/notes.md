# auto-sqlite-2fa1ffbe63

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#9de75a1245e3d6a66b8283673719f92830a38c3c](https://github.com/sqlite/sqlite/commit/9de75a1245e3d6a66b8283673719f92830a38c3c) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-13 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 13 |
| 编译错误数（gcc syntax-only） | 20（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #9de75a1245e3d6a66b8283673719f92830a38c3c (https://github.com/sqlite/sqlite/commit/9de75a1245e3d6a66b8283673719f92830a38c3c)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1888；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 9de75a1245e3d6a66b8283673719f92830a38c3c Merge fixes from branch-3.54 onto trunk. :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1736,10 +1736,8 @@ static int vdbeUnbind(Vdbe *p, unsigned int i){
     sqlite3_mutex_leave(p->db->mutex);
     return SQLITE_RANGE;
   }
+
   pVar = &p->aVar[i];
-  sqlite3VdbeMemRelease(pVar);
-  pVar->flags = MEM_Null;
-  p->db->errCode = SQLITE_OK;
 
   /* If the bit corresponding to this variable in Vdbe.expmask is set, then
   ** binding a new value to this variable invalidates the current query plan.
@@ -1753,11 +1751,20 @@ static int vdbeUnbind(Vdbe *p, unsigned int i){
   assert( (p->prepFlags & SQLITE_PREPARE_SAVESQL)!=0 || p->expmask==0 );
   assert( (p->expmask & p->smimask)==p->smimask );
   if( p->expmask!=0 && (p->expmask & (i>=31 ? 0x80000000 : (u32)1<<i))!=0 ){
-    /* We might avoid a reprepare here if p->smimask is set and the old
-    ** value is an integer other than (0,1).  But that is such a corner
-    ** case that it does not seem worth the extra code to implement. */
-    p->expired = 1;
+    /* If the p->smimask bit is also set, then only expire the statement if
+    ** the value being clobbered was integer value 0 or 1. */
+    if( (p->smimask==0) 
+     || (p->smimask & (i>=31 ? 0x80000000 : (u32)1<<i))==0 
+     || ((pVar->flags & MEM_Int)!=0 && pVar->u.i>=0 && pVar->u.i<=1)
+    ){
+      p->expired = 1;
+    }
   }
+
+  sqlite3VdbeMemRelease(pVar);
+  pVar->flags = MEM_Null;
+  p->db->errCode = SQLITE_OK;
+
   return SQLITE_OK;
 }
 
@@ -1885,8 +1892,8 @@ int sqlite3_bind_int64(sqlite3_stmt *pStmt, int i, sqlite_int64 iValue){
         if( sm1!=sm2 ){
           p->expired = 1;
         }
-      }else if( (pVar->flags & MEM_Int)==0 || pVar->u.i!=iValue ){
-        /* Always expire if the value really is changing */
+      }else if( (pVar->flags & MEM_Int)==0 || iValue!=pVar->u.i ){
+        /* Always expire if the smimask bit is clear */
         p->expired = 1;
       }
     }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`sqlite3VdbeMemRelease`
- 外部函数：`sqlite3_mutex_leave`
- 外部函数：`than`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_PREPARE_SAVESQL`
- 大写宏：`SQLITE_RANGE`
- 外部类型：`Always`
- 外部类型：`But`
- 外部类型：`If`
- 外部类型：`MEM_Int`
- 外部类型：`MEM_Null`
- 外部类型：`Vdbe`
- 外部类型：`We`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-2fa1ffbe63` → 本草稿移入 `cases/defect/auto-sqlite-2fa1ffbe63/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
