# auto-sqlite-c23c0dcee1

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#5ee733befea57e6a12943273fe98b4f3d722b8f6](https://github.com/sqlite/sqlite/commit/5ee733befea57e6a12943273fe98b4f3d722b8f6) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-06 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 61 |
| 编译错误数（gcc syntax-only） | 14（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #5ee733befea57e6a12943273fe98b4f3d722b8f6 (https://github.com/sqlite/sqlite/commit/5ee733befea57e6a12943273fe98b4f3d722b8f6)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 5ee733befea57e6a12943273fe98b4f3d722b8f6 Allow windows clients to open and recover a wal mode database even if an old client still has the *-shm file mapped. For :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -8363,6 +8363,163 @@ static int SQLITE_TCLAPI win32_file_lock(
 }
 #endif
 
+#ifdef _WIN32
+/*
+** An instance of this structure is allocated for each command created
+** by [map_win32_file].
+*/
+typedef struct TestMapFile TestMapFile;
+struct TestMapFile {
+  HANDLE hFile;                   /* Handle open on mapped file */
+  HANDLE hMap;                    /* Handle returned by CreateFileMappingW() */
+  void *pMap;                     /* Pointer returned by MapViewOfFile() */
+};
+
+/*
+** Release all resources held by the TestMapFile object passed as the
+** only argument. This is used as the delete-proc for the Tcl command.
+*/
+static void SQLITE_TCLAPI testMapFileDel(void *clientData){
+  TestMapFile *p = (TestMapFile*)clientData;
+  if( p->pMap ) UnmapViewOfFile(p->pMap);
+  if( p->hMap ) CloseHandle(p->hMap);
+  if( p->hFile!=INVALID_HANDLE_VALUE ) CloseHandle(p->hFile);
+  ckfree((char*)p);
+}
+
+/*
+** Implementation of the command created by [map_win32_file]:
+**
+**      CMD destroy
+*/
+static int SQLITE_TCLAPI testMapFileCmd(
+  void *clientData,
+  Tcl_Interp *interp,
+  int objc,
+  Tcl_Obj *CONST objv[]
+){
+  static const char *azMethod[] = { "destroy", 0 };
+  enum { MAPFILE_DESTROY };
+  int iMethod = 0;
+
+  if( objc<2 ){
+    Tcl_WrongNumArgs(interp, 1, objv, "SUB-COMMAND ...");
+    return TCL_ERROR;
+  }
+  if( Tcl_GetIndexFromObj(interp, objv[1], azMethod, "method", 0, &iMethod) ){
+    return TCL_ERROR;
+  }
+
+  switch( iMethod ){
+    case MAPFILE_DESTROY: {
+      if( objc!=2 ){
+        Tcl_WrongNumArgs(interp, 2, objv, "");
+        return TCL_ERROR;
+      }
+      Tcl_DeleteCommand(interp, Tcl_GetString(objv[0]));
+      break;
+    }
+  }
+
+  return TCL_OK;
+}
+
+/*
+**      map_win32_file CMD FILENAME
+**
+** Open file FILENAME and memory-map the entire file using the same
+** CreateFileMappingW() and MapViewOfFile() APIs used by winShmMap() in
+** os_win.c. Create a new Tcl command named CMD that may be used to
+** unmap the file ([CMD destroy]).
+*/
+static int SQLITE_TCLAPI map_win32_file(
+  void * clientData,
+  Tcl_Interp *interp,
+  int objc,
+  Tcl_Obj *CONST objv[]
+){
+  const char *zCmd = 0;
+  const char *zFile = 0;
+  LPWSTR zWide = 0;
+  int nWide = 0;
+  TestMapFile *p = 0;
+  LARGE_INTEGER sz;
+  char zErr[100];
+
+  if( objc!=3 ){
+    Tcl_WrongNumArgs(interp, 1, objv, "CMD FILENAME");
+    return TCL_ERROR;
+  }
+  zCmd = Tcl_GetString(objv[1]);
+  zFile = Tcl_GetString(objv[2]);
+
+  /* Convert the utf-8 filename to utf-16 */
+  nWide = MultiByteToWideChar(CP_UTF8, 0, zFile, -1, NULL, 0);
+  if( nWide<=0 ){
+    Tcl_AppendResult(interp, "cannot convert filename: ", zFile, (char*)0);
+    return TCL_ERROR;
+  }
+  zWide = (LPWSTR)ckalloc(nWide * sizeof(WCHAR));
+  MultiByteToWideChar(CP_UTF8, 0, zFile, -1, zWide, nWide);
+
+  p = (TestMapFile*)ckalloc(sizeof(TestMapFile));
+  memset(p, 0, sizeof(TestMapFile));
+  p->hFile = CreateFileW(zWide,
+      GENERIC_READ | GENERIC_WRITE,
+      FILE_SHARE_READ | FILE_SHARE_WRITE,
+      NULL,
+      OPEN_EXISTING,
+      FILE_ATTRIBUTE_NORMAL,
+      NULL
+  );
+  if( p->hFile==INVALID_HANDLE_VALUE ){
+    DWORD lastErr = GetLastError();
+    ckfree((char*)zWide);
+    sqlite3_snprintf(sizeof(zErr), zErr, " (%lu)", lastErr);
+    Tcl_AppendResult(interp, "cannot open file: ", zFile, zErr, (char*)0);
+    testMapFileDel((void*)p);
+    return TCL_ERROR;
+  }
+  ckfree((char*)zWide);
+
+  if( !GetFileSizeEx(p->hFile, &sz) ){
+    sqlite3_snprintf(sizeof(zErr), zErr, " (%lu)", GetLastError());
+    Tcl_AppendResult(interp, "GetFileSizeEx failed", zErr, (char*)0);
+    testMapFileDel((void*)p);
+    return TCL_ERROR;
+  }
+  if( sz.QuadPart==0 ){
+    Tcl_AppendResult(interp, "cannot map zero-length file: ", zFile, (char*)0);
+    testMapFileDel((void*)p);
+    return TCL_ERROR;
+  }
+
+  /* Map the entire file. Same protection and flags as winShmMap(). */
+  p->hMap = CreateFileMappingW(p->hFile, NULL, PAGE_READWRITE,
+      (DWOR
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`_beginthread`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`objProc`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_malloc`
- 外部函数：`sqlite3_snprintf`
- 外部函数：`sqlite3_vfs_find`
- 外部函数：`xClose`
- 外部函数：`xFileControl`
- 外部函数：`xOpen`
- 大写宏：`BOOLEAN`
- 大写宏：`COMMAND`
- 大写宏：`CONST`
- 大写宏：`DELAY1`
- 大写宏：`DELAY2`
- 大写宏：`DWORD`
- 大写宏：`EVENT_MODIFY_STATE`
- 大写宏：`FALSE`
- 大写宏：`FILENAME`
- 大写宏：`HANDLE`
- 大写宏：`INVALID_HANDLE_VALUE`
- 大写宏：`NULL`
- 大写宏：`OPT`
- 大写宏：`SQLITE_CDECL`
- 大写宏：`SQLITE_FCNTL_WIN32_GET_HANDLE`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_OPEN_MAIN_DB`
- 大写宏：`SQLITE_OPEN_READWRITE`
- 大写宏：`SQLITE_TCLAPI`
- 大写宏：`TCL_ERROR`
- 大写宏：`TCL_OK`
- 大写宏：`TRUE`
- 大写宏：`WAIT_OBJECT_0`
- 外部类型：`Delay`
- 外部类型：`Finished`
- 外部类型：`Handle`
- 外部类型：`Name`
- 外部类型：`SQLITE_AllOpts`
- 外部类型：`SQLITE_CoverIdxScan`
- 外部类型：`SQLITE_DistinctOpt`
- 外部类型：`SQLITE_GroupByOrder`
- 外部类型：`SQLITE_OmitNoopJoin`
- 外部类型：`SQLITE_OrderByIdxJoin`
- 外部类型：`SQLITE_OrderBySubq`
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
- 外部类型：`Tcl_Size`
- 外部类型：`True`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-c23c0dcee1` → 本草稿移入 `cases/defect/auto-sqlite-c23c0dcee1/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
