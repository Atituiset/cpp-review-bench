# auto-sqlite-729ae1bf43

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
| 外部依赖数（dep_count） | 24 |
| 编译错误数（gcc syntax-only） | 8（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #5ee733befea57e6a12943273fe98b4f3d722b8f6 (https://github.com/sqlite/sqlite/commit/5ee733befea57e6a12943273fe98b4f3d722b8f6)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 9（原始 PR diff 行 3107；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 5ee733befea57e6a12943273fe98b4f3d722b8f6 Allow windows clients to open and recover a wal mode database even if an old client still has the *-shm file mapped. For :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3086,6 +3086,28 @@ static void winShmPurge(sqlite3_vfs *pVfs, int deleteFlag){
   }
 }
 
+/*
+** Handle h is open on the *-shm file. This function zeroes the first
+** 8 bytes of the file. If SQLITE_ENABLE_SETLK_TIMEOUT is defined, h may 
+** have been opened with FILE_FLAG_OVERLAPPED.
+*/
+static int winZeroSharedMemory(HANDLE h){
+  u8 aZero[8] = {0,0,0,0,0,0,0,0};
+  DWORD nWrite = 0;
+  OVERLAPPED ovlp;
+  memset(&ovlp, 0, sizeof(OVERLAPPED));
+  if( !osWriteFile(h, aZero, sizeof(aZero), &nWrite, &ovlp) ){
+    /* If the file was opened with FILE_FLAG_OVERLAPPED, the write may still
+    ** be pending. Wait for it with GetOverlappedResult().  */
+    if( osGetLastError()!=ERROR_IO_PENDING
+     || !GetOverlappedResult(h, &ovlp, &nWrite, TRUE)
+    ){
+      nWrite = 0;
+    }
+  }
+  return (nWrite==sizeof(aZero) ? SQLITE_OK : SQLITE_IOERR_WRITE);
+}
+
 /*
 ** The DMS lock has not yet been taken on the shm file associated with
 ** pShmNode. Take the lock. Truncate the *-shm file if required.
@@ -3104,7 +3126,12 @@ static int winLockSharedMemory(winShmNode *pShmNode, DWORD nMs){
     if( pShmNode->isReadonly ){
       rc = SQLITE_READONLY_CANTINIT;
     }else{
-      rc = winHandleTruncate(h, 0);
+      if( SQLITE_OK!=winHandleTruncate(h, 0) ){
+        /* Sometimes the truncate operation may fail because some other
+        ** process is still holding an open mapping. So try to write
+        ** zeroes into the start of the file instead. */
+        rc = winZeroSharedMemory(h);
+      }
     }
 
     /* Release the EXCLUSIVE lock acquired above. */
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`osSetFilePointerEx`
- 外部函数：`sqlite3ErrName`
- 外部函数：`winHandleSeek`
- 大写宏：`BOOL`
- 大写宏：`DMS`
- 大写宏：`DWORD`
- 大写宏：`EXCLUSIVE`
- 大写宏：`FILE_BEGIN`
- 大写宏：`HANDLE`
- 大写宏：`LARGE_INTEGER`
- 大写宏：`OSTRACE`
- 大写宏：`PLARGE_INTEGER`
- 大写宏：`SEEK`
- 大写宏：`SQLITE_IOERR_SEEK`
- 大写宏：`SQLITE_IOERR_TRUNCATE`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_READONLY_CANTINIT`
- 大写宏：`WINAPI`
- 外部类型：`QuadPart`
- 外部类型：`Release`
- 外部类型：`Return`
- 外部类型：`Take`
- 外部类型：`The`
- 外部类型：`Truncate`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-729ae1bf43` → 本草稿移入 `cases/defect/auto-sqlite-729ae1bf43/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
