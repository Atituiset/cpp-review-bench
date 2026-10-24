# auto-sqlite-2f649c0d6e

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#196f6a9389c2dd1c1b4d350deb5fac2873c77003](https://github.com/sqlite/sqlite/commit/196f6a9389c2dd1c1b4d350deb5fac2873c77003) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-24 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 55 |
| 编译错误数（gcc syntax-only） | 29（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #196f6a9389c2dd1c1b4d350deb5fac2873c77003 (https://github.com/sqlite/sqlite/commit/196f6a9389c2dd1c1b4d350deb5fac2873c77003)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 3273；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 196f6a9389c2dd1c1b4d350deb5fac2873c77003 Fix a race condition with a writer that could cause a connection with a read-only *-shm file to return a spurious SQLITE :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3270,25 +3270,35 @@ static int walTryBeginRead(Wal *pWal, int *pChanged, int useWal, int *pCnt){
         mxI = i;
       }
     }
-    if( (pWal->readOnly & WAL_SHM_RDONLY)==0
-     && (mxReadMark<mxFrame || mxI==0)
-    ){
-      for(i=1; i<WAL_NREADER; i++){
-        rc = walLockExclusive(pWal, WAL_READ_LOCK(i), 1);
-        if( rc==SQLITE_OK ){
-          AtomicStore(pInfo->aReadMark+i,mxFrame);
-          mxReadMark = mxFrame;
-          mxI = i;
-          walUnlockExclusive(pWal, WAL_READ_LOCK(i), 1);
-          break;
-        }else if( rc!=SQLITE_BUSY ){
-          return rc;
+
+    if( (pWal->readOnly & WAL_SHM_RDONLY)==0 ){
+      if( mxReadMark<mxFrame || mxI==0 ){
+        for(i=1; i<WAL_NREADER; i++){
+          rc = walLockExclusive(pWal, WAL_READ_LOCK(i), 1);
+          if( rc==SQLITE_OK ){
+            AtomicStore(pInfo->aReadMark+i,mxFrame);
+            mxReadMark = mxFrame;
+            mxI = i;
+            walUnlockExclusive(pWal, WAL_READ_LOCK(i), 1);
+            break;
+          }else if( rc!=SQLITE_BUSY ){
+            return rc;
+          }
         }
       }
+    }else if( mxI==0 ){
+      /* If the loop above could not find a usable slot (mxI==0), then check
+      ** if the wal-index header has been modified by another connection since
+      ** it was read. If not, return SQLITE_READONLY_CANTINIT. If it has
+      ** been modified, fall through and return WAL_RETRY.  */
+      walShmBarrier(pWal);
+      if( 0==memcmp((void*)walIndexHdr(pWal),&pWal->hdr,sizeof(WalIndexHdr)) ){
+        return SQLITE_READONLY_CANTINIT;
+      }
     }
+
     if( mxI==0 ){
-      assert( rc==SQLITE_BUSY || (pWal->readOnly & WAL_SHM_RDONLY)!=0 );
-      return rc==SQLITE_BUSY ? WAL_RETRY : SQLITE_READONLY_CANTINIT;
+      return WAL_RETRY;
     }
   
     (void)walEnableBlockingMs(pWal, nBlockTmout);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`callback`
- 外部函数：`sqlite3OsShmLock`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_get_snapshot`
- 外部函数：`sqlite3_snprintf`
- 外部函数：`walEnableBlockingMs`
- 大写宏：`CKPT`
- 大写宏：`EXCLUSIVE`
- 大写宏：`LOCK`
- 大写宏：`NULL`
- 大写宏：`READ`
- 大写宏：`RECOVER`
- 大写宏：`SEH_TRY`
- 大写宏：`SHM`
- 大写宏：`SQLITE_BUSY`
- 大写宏：`SQLITE_DEBUG`
- 大写宏：`SQLITE_ENABLE_SETLK_TIMEOUT`
- 大写宏：`SQLITE_ENABLE_SNAPSHOT`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_READONLY_CANTINIT`
- 大写宏：`SQLITE_SHM_EXCLUSIVE`
- 大写宏：`SQLITE_SHM_LOCK`
- 大写宏：`SQLITE_SHM_NLOCK`
- 大写宏：`SQLITE_SHM_UNLOCK`
- 大写宏：`SQLITE_USE_SEH`
- 大写宏：`VFS`
- 大写宏：`VVA_ONLY`
- 大写宏：`WAL`
- 大写宏：`WAL_RDONLY`
- 大写宏：`WAL_RDWR`
- 大写宏：`WRITE`
- 外部类型：`Checkpoint`
- 外部类型：`Database`
- 外部类型：`File`
- 外部类型：`Flags`
- 外部类型：`Fsync`
- 外部类型：`Ignore`
- 外部类型：`Mask`
- 外部类型：`Name`
- 外部类型：`Non`
- 外部类型：`Number`
- 外部类型：`On`
- 外部类型：`Pad`
- 外部类型：`Pointer`
- 外部类型：`Size`
- 外部类型：`Start`
- 外部类型：`System`
- 外部类型：`The`
- 外部类型：`Transaction`
- 外部类型：`True`
- 外部类型：`Truncate`
- 外部类型：`Value`
- 外部类型：`Wal`
- 外部类型：`WalIndexHdr`
- 外部类型：`Which`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-2f649c0d6e` → 本草稿移入 `cases/defect/auto-sqlite-2f649c0d6e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
