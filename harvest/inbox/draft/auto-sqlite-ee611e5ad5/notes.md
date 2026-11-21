# auto-sqlite-ee611e5ad5

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#7dd00550bf588547c66f1084f531d91b28805910](https://github.com/sqlite/sqlite/commit/7dd00550bf588547c66f1084f531d91b28805910) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-21 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 24 |
| 编译错误数（gcc syntax-only） | 29（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #7dd00550bf588547c66f1084f531d91b28805910 (https://github.com/sqlite/sqlite/commit/7dd00550bf588547c66f1084f531d91b28805910)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 7dd00550bf588547c66f1084f531d91b28805910 Disable the SQLITE_DBCONFIG_FP_DIGITS settings, as it causes far more problems :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -8172,7 +8172,8 @@ static int SQLITE_TCLAPI test_test_control(
     { "SQLITE_TESTCTRL_SORTER_MMAP",        SQLITE_TESTCTRL_SORTER_MMAP     }, 
     { "SQLITE_TESTCTRL_IMPOSTER",           SQLITE_TESTCTRL_IMPOSTER        },
     { "SQLITE_TESTCTRL_INTERNAL_FUNCTIONS", SQLITE_TESTCTRL_INTERNAL_FUNCTIONS},
-    { "SQLITE_TESTCTRL_FK_NO_ACTION",       SQLITE_TESTCTRL_FK_NO_ACTION},
+    { "SQLITE_TESTCTRL_FK_NO_ACTION",       SQLITE_TESTCTRL_FK_NO_ACTION    },
+    { "SQLITE_TESTCTRL_FP_DIGITS",          SQLITE_TESTCTRL_FP_DIGITS       },
     { 0, 0 }
   };
   int iVerb;
@@ -8201,6 +8202,22 @@ static int SQLITE_TCLAPI test_test_control(
       sqlite3_test_control(SQLITE_TESTCTRL_INTERNAL_FUNCTIONS, db);
       break;
     }
+    case SQLITE_TESTCTRL_FP_DIGITS: {
+      sqlite3 *db = 0;
+      int nIn = 0;
+      int nOut = 0;
+      if( objc<3 || objc>4 ){
+        Tcl_WrongNumArgs(interp, 2, objv, "DB ?NDIGIT?");
+        return TCL_ERROR;
+      }
+      if( getDbPointer(interp, Tcl_GetString(objv[2]), &db) ) return TCL_ERROR;
+      if( objc==4 ){
+        if( Tcl_GetIntFromObj(interp, objv[3], &nIn) ) return TCL_ERROR;
+      }
+      sqlite3_test_control(SQLITE_TESTCTRL_FP_DIGITS, db, nIn, &nOut);
+      Tcl_SetObjResult(interp, Tcl_NewIntObj(nOut));
+      break;
+    }
     case SQLITE_TESTCTRL_LOCALTIME_FAULT: {
       int val;
       if( objc!=3 ){
@@ -8221,7 +8238,6 @@ static int SQLITE_TCLAPI test_test_control(
       }
       if( getDbPointer(interp, Tcl_GetString(objv[2]), &db) ) return TCL_ERROR;
       if( Tcl_GetBooleanFromObj(interp, objv[3], &val) ) return TCL_ERROR;
-
       sqlite3_test_control(SQLITE_TESTCTRL_FK_NO_ACTION, db, val);
       break;
     }
@@ -8933,7 +8949,6 @@ static int SQLITE_TCLAPI test_sqlite3_db_config(
     { "ATTACH_CREATE",      SQLITE_DBCONFIG_ENABLE_ATTACH_CREATE },
     { "ATTACH_WRITE",       SQLITE_DBCONFIG_ENABLE_ATTACH_WRITE },
     { "COMMENTS",           SQLITE_DBCONFIG_ENABLE_COMMENTS },
-    { "FP_DIGITS",          SQLITE_DBCONFIG_FP_DIGITS },
   };
   int i;
   int v = 0;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`sqlite3_mutex_alloc`
- 外部函数：`sqlite3_mutex_enter`
- 外部函数：`sqlite3_mutex_leave`
- 外部函数：`sqlite3_test_control`
- 大写宏：`ATTACH_CREATE`
- 大写宏：`ATTACH_WRITE`
- 大写宏：`COMMENTS`
- 大写宏：`FP_DIGITS`
- 大写宏：`SQLITE_DBCONFIG_ENABLE_ATTACH_CREATE`
- 大写宏：`SQLITE_DBCONFIG_ENABLE_ATTACH_WRITE`
- 大写宏：`SQLITE_DBCONFIG_ENABLE_COMMENTS`
- 大写宏：`SQLITE_DBCONFIG_FP_DIGITS`
- 大写宏：`SQLITE_MUTEX_STATIC_APP3`
- 大写宏：`SQLITE_TESTCTRL_FK_NO_ACTION`
- 大写宏：`SQLITE_TESTCTRL_IMPOSTER`
- 大写宏：`SQLITE_TESTCTRL_INTERNAL_FUNCTIONS`
- 大写宏：`SQLITE_TESTCTRL_LOCALTIME_FAULT`
- 大写宏：`SQLITE_TESTCTRL_SORTER_MMAP`
- 大写宏：`TCL_ERROR`
- 大写宏：`TCL_OK`
- 外部类型：`SqliteDb`
- 外部类型：`Tcl_CmdInfo`
- 外部类型：`Tcl_HashEntry`
- 外部类型：`Tcl_Interp`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-ee611e5ad5` → 本草稿移入 `cases/defect/auto-sqlite-ee611e5ad5/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
