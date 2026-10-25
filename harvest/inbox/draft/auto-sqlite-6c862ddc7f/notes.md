# auto-sqlite-6c862ddc7f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#30be364a7c4fd8e30249ceea7b5054f134656e02](https://github.com/sqlite/sqlite/commit/30be364a7c4fd8e30249ceea7b5054f134656e02) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-25 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 52 |
| 编译错误数（gcc syntax-only） | 37（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #30be364a7c4fd8e30249ceea7b5054f134656e02 (https://github.com/sqlite/sqlite/commit/30be364a7c4fd8e30249ceea7b5054f134656e02)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 30be364a7c4fd8e30249ceea7b5054f134656e02 Always report back the output flags from the xOpen method of the unix VFS. :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -6705,6 +6705,52 @@ static int SQLITE_TCLAPI working_64bit_int(
 }
 
 
+/*
+** tclcmd:   vfs_open_tempfile ?VFS?
+**
+** Invoke the xOpen() method of VFS (or of the default VFS if no VFS is
+** named) with a NULL filename, as the core does when it needs a temporary
+** file.  The file is closed again immediately.  Return a two-element list
+** consisting of the integer return code from xOpen() and the value that
+** xOpen() wrote into *pOutFlags.  *pOutFlags is initialized to zero before
+** the call, so a result of zero means that xOpen() did not set it.
+*/
+static int SQLITE_TCLAPI vfs_open_tempfile(
+  ClientData clientData, /* Not used */
+  Tcl_Interp *interp,    /* The TCL interpreter that invoked this command */
+  int objc,              /* Number of arguments */
+  Tcl_Obj *CONST objv[]  /* Command arguments */
+){
+  sqlite3_vfs *pVfs;
+  sqlite3_file *pFd;
+  int flags = SQLITE_OPEN_TEMP_DB | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE
+            | SQLITE_OPEN_EXCLUSIVE | SQLITE_OPEN_DELETEONCLOSE;
+  int outFlags = 0;
+  int rc;
+  Tcl_Obj *pRet;
+
+  if( objc!=1 && objc!=2 ){
+    Tcl_WrongNumArgs(interp, 1, objv, "?VFS?");
+    return TCL_ERROR;
+  }
+  pVfs = sqlite3_vfs_find(objc==2 ? Tcl_GetString(objv[1]) : 0);
+  if( pVfs==0 ){
+    Tcl_AppendResult(interp, "no such vfs", (char*)0);
+    return TCL_ERROR;
+  }
+  pFd = (sqlite3_file*)ckalloc(pVfs->szOsFile);
+  memset(pFd, 0, pVfs->szOsFile);
+  rc = pVfs->xOpen(pVfs, 0, pFd, flags, &outFlags);
+  if( pFd->pMethods ) pFd->pMethods->xClose(pFd);
+  ckfree((char*)pFd);
+
+  pRet = Tcl_NewObj();
+  Tcl_ListObjAppendElement(interp, pRet, Tcl_NewIntObj(rc));
+  Tcl_ListObjAppendElement(interp, pRet, Tcl_NewIntObj(outFlags));
+  Tcl_SetObjResult(interp, pRet);
+  return TCL_OK;
+}
+
 /*
 ** tclcmd:   vfs_unlink_test
 **
@@ -9481,6 +9527,7 @@ int Sqlitetest1_Init(Tcl_Interp *interp){
      { "sqlite3_create_collation_v2", test_create_collation_v2, 0 },
      { "sqlite3_global_recover",     test_global_recover, 0   },
      { "working_64bit_int",          working_64bit_int,   0   },
+     { "vfs_open_tempfile",          vfs_open_tempfile,   0   },
      { "vfs_unlink_test",            vfs_unlink_test,     0   },
      { "vfs_initfail_test",          vfs_initfail_test,   0   },
      { "vfs_unregister_all",         vfs_unregister_all,  0   },
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`sqlite3_create_collate_v2`
- 外部函数：`sqlite3_create_collation_v2`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_global_recover`
- 外部函数：`sqlite3_malloc`
- 外部函数：`sqlite3_mutex_alloc`
- 外部函数：`sqlite3_mutex_enter`
- 外部函数：`sqlite3_mutex_leave`
- 外部函数：`sqlite3_vfs_find`
- 外部函数：`sqlite3_vfs_register`
- 外部函数：`sqlite3_vfs_unregister`
- 外部函数：`strcmp`
- 外部函数：`t1ErrorName`
- 大写宏：`CMP`
- 大写宏：`CONST`
- 大写宏：`DEL`
- 大写宏：`HANDLE`
- 大写宏：`NAME`
- 大写宏：`NULL`
- 大写宏：`PROC`
- 大写宏：`SQLITE_MISUSE`
- 大写宏：`SQLITE_MUTEX_STATIC_APP3`
- 大写宏：`SQLITE_OMIT_DEPRECATED`
- 大写宏：`SQLITE_TCLAPI`
- 大写宏：`SQLITE_UTF8`
- 大写宏：`TCL`
- 大写宏：`TCL_ERROR`
- 大写宏：`TCL_EVAL_DIRECT`
- 大写宏：`TCL_EVAL_GLOBAL`
- 大写宏：`TCL_OK`
- 大写宏：`TCL_STATIC`
- 大写宏：`VFS`
- 外部类型：`Calling`
- 外部类型：`ClientData`
- 外部类型：`Command`
- 外部类型：`Not`
- 外部类型：`Number`
- 外部类型：`Pointer`
- 外部类型：`SqliteDb`
- 外部类型：`Tcl_CmdInfo`
- 外部类型：`Tcl_HashEntry`
- 外部类型：`Tcl_Interp`
- 外部类型：`Tcl_Obj`
- 外部类型：`TestCollationX`
- 外部类型：`The`
- 外部类型：`Unregister`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-6c862ddc7f` → 本草稿移入 `cases/defect/auto-sqlite-6c862ddc7f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
