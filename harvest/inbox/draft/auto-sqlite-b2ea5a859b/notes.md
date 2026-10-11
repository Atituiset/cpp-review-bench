# auto-sqlite-b2ea5a859b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#4c7729af99a9fbf54c812cbd6b14572ec8680963](https://github.com/sqlite/sqlite/commit/4c7729af99a9fbf54c812cbd6b14572ec8680963) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-11 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 29 |
| 编译错误数（gcc syntax-only） | 6（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #4c7729af99a9fbf54c812cbd6b14572ec8680963 (https://github.com/sqlite/sqlite/commit/4c7729af99a9fbf54c812cbd6b14572ec8680963)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 8542；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 4c7729af99a9fbf54c812cbd6b14572ec8680963 New extension ext/misc/optname.c provides the canonical mapping between :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -8539,35 +8539,15 @@ static int SQLITE_TCLAPI optimization_control(
   int objc,
   Tcl_Obj *CONST objv[]
 ){
-  int i;
+  size_t i, j, k;
   sqlite3 *db;
   const char *zOpt;
   int onoff;
-  int mask = 0;
-  int cnt = 0;
-  static const struct {
-    const char *zOptName;
-    int mask;
-  } aOpt[] = {
-    { "all",                 SQLITE_AllOpts        },
-    { "none",                0                     },
-    { "query-flattener",     SQLITE_QueryFlattener },
-    { "groupby-order",       SQLITE_GroupByOrder   },
-    { "distinct-opt",        SQLITE_DistinctOpt    },
-    { "cover-idx-scan",      SQLITE_CoverIdxScan   },
-    { "order-by-idx-join",   SQLITE_OrderByIdxJoin },
-    { "order-by-subquery",   SQLITE_OrderBySubq    },
-    { "transitive",          SQLITE_Transitive     },
-    { "omit-noop-join",      SQLITE_OmitNoopJoin   },
-    { "stat4",               SQLITE_Stat4          },
-    { "skip-scan",           SQLITE_SkipScan       },
-    { "push-down",           SQLITE_PushDown       },
-    { "balanced-merge",      SQLITE_BalancedMerge  },
-    { "propagate-const",     SQLITE_PropagateConst },
-    { "one-pass",            SQLITE_OnePass        },
-    { "exists-to-join",      SQLITE_ExistsToJoin   },
-    { "count-of-view",       SQLITE_CountOfView    },
-  };
+  u64 m = 0, mask = 0;
+  int m32;
+  char zBuf[50];
+  extern int sqlite3_optimization_mask(const char*,sqlite3_uint64*);
+  extern const char *sqlite3_optimization_name(sqlite3_uint64);
 
   if( objc!=4 ){
     Tcl_WrongNumArgs(interp, 1, objv, "DB OPT BOOLEAN");
@@ -8576,23 +8556,36 @@ static int SQLITE_TCLAPI optimization_control(
   if( getDbPointer(interp, Tcl_GetString(objv[1]), &db) ) return TCL_ERROR;
   if( Tcl_GetBooleanFromObj(interp, objv[3], &onoff) ) return TCL_ERROR;
   zOpt = Tcl_GetString(objv[2]);
-  for(i=0; i<sizeof(aOpt)/sizeof(aOpt[0]); i++){
-    if( strstr(zOpt, aOpt[i].zOptName)!=0 ){
-      mask |= aOpt[i].mask;
-      cnt++;
+  while( zOpt && zOpt[0] ){
+    while( zOpt[0]==' ' || zOpt[0]==',' ) zOpt++;
+    for(i=0; zOpt[i] && zOpt[i]!=',' && zOpt[i]!=' '; i++){}
+    if( i==0 ) break;
+    if( i<sizeof(zBuf)-1 ){
+      for(j=k=0; j<i; j++){
+        if( zOpt[j]!='-' ) zBuf[k++] = zOpt[j];
+      }
+      zBuf[k] = 0;
+      if( sqlite3_optimization_mask(zBuf, &m)==SQLITE_NOTFOUND ){
+        const char *zName;
+        Tcl_AppendResult(interp, "unknown optimization \"", zBuf,
+                         "\" - should be one of:", (char*)0);
+        for(i=0; i<64; i++){
+          m = 1ull<<i;
+          zName = sqlite3_optimization_name(m);
+          if( zName ){
+            Tcl_AppendResult(interp, " ", zName, (char*)0);
+          }
+        }
+        return TCL_ERROR;
+      }
+      mask |= m;
     }
+    zOpt += i;
   }
   if( onoff ) mask = ~mask;
-  if( cnt==0 ){
-    Tcl_AppendResult(interp, "unknown optimization - should be one of:",
-                     (char*)0);
-    for(i=0; i<sizeof(aOpt)/sizeof(aOpt[0]); i++){
-      Tcl_AppendResult(interp, " ", aOpt[i].zOptName, (char*)0);
-    }
-    return TCL_ERROR;
-  }
-  sqlite3_test_control(SQLITE_TESTCTRL_OPTIMIZATIONS, db, mask);
-  Tcl_SetObjResult(interp, Tcl_NewIntObj(mask));
+  m32 = (int)(mask & 0xffffffff);
+  sqlite3_test_control(SQLITE_TESTCTRL_OPTIMIZATIONS,db, m32);
+  Tcl_SetObjResult(interp, Tcl_NewIntObj(m32));
   return TCL_OK;
 }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`sqlite3_test_control`
- 外部函数：`strstr`
- 大写宏：`BOOLEAN`
- 大写宏：`CONST`
- 大写宏：`OPT`
- 大写宏：`SQLITE_TESTCTRL_OPTIMIZATIONS`
- 大写宏：`TCL_ERROR`
- 大写宏：`TCL_OK`
- 外部类型：`SQLITE_AllOpts`
- 外部类型：`SQLITE_BalancedMerge`
- 外部类型：`SQLITE_CountOfView`
- 外部类型：`SQLITE_CoverIdxScan`
- 外部类型：`SQLITE_DistinctOpt`
- 外部类型：`SQLITE_ExistsToJoin`
- 外部类型：`SQLITE_GroupByOrder`
- 外部类型：`SQLITE_OmitNoopJoin`
- 外部类型：`SQLITE_OnePass`
- 外部类型：`SQLITE_OrderByIdxJoin`
- 外部类型：`SQLITE_OrderBySubq`
- 外部类型：`SQLITE_PropagateConst`
- 外部类型：`SQLITE_PushDown`
- 外部类型：`SQLITE_QueryFlattener`
- 外部类型：`SQLITE_SkipScan`
- 外部类型：`SQLITE_Stat4`
- 外部类型：`SQLITE_Transitive`
- 外部类型：`SqliteDb`
- 外部类型：`Tcl_CmdInfo`
- 外部类型：`Tcl_HashEntry`
- 外部类型：`Tcl_Interp`
- 外部类型：`Tcl_Obj`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-b2ea5a859b` → 本草稿移入 `cases/defect/auto-sqlite-b2ea5a859b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
