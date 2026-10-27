# auto-sqlite-dd7c51ff11

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#77f8caa273d6d30f3f0e278b2ac04715a87b37df](https://github.com/sqlite/sqlite/commit/77f8caa273d6d30f3f0e278b2ac04715a87b37df) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-27 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 27 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #77f8caa273d6d30f3f0e278b2ac04715a87b37df (https://github.com/sqlite/sqlite/commit/77f8caa273d6d30f3f0e278b2ac04715a87b37df)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 8（原始 PR diff 行 248；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 77f8caa273d6d30f3f0e278b2ac04715a87b37df Merge corrections from branch-3.54 into trunk :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -245,7 +245,7 @@ struct sqlite3_value {
   /* ShallowCopy only needs to copy the information above */
   sqlite3 *db;        /* The associated database connection */
   int szMalloc;       /* Size of the zMalloc allocation */
-  tSerial uTemp;      /* Transient storage for serial_type in OP_MakeRecord */
+  u32 uTemp;          /* Transient storage for serial_type in OP_MakeRecord */
   char *zMalloc;      /* Space to hold MEM_Str or MEM_Blob if szMalloc>0 */
   void (*xDel)(void*);/* Destructor for Mem.z - only valid if MEM_Dyn */
 #ifdef SQLITE_DEBUG
@@ -610,15 +610,15 @@ void sqliteVdbePopStack(Vdbe*,int);
 int SQLITE_NOINLINE sqlite3VdbeHandleMovedCursor(VdbeCursor *p);
 int SQLITE_NOINLINE sqlite3VdbeFinishMoveto(VdbeCursor*);
 int sqlite3VdbeCursorRestore(VdbeCursor*);
-u32 sqlite3VdbeSerialTypeLen(tSerial);
+u32 sqlite3VdbeSerialTypeLen(u32);
 u8 sqlite3VdbeOneByteSerialTypeLen(u8);
 #ifdef SQLITE_MIXED_ENDIAN_64BIT_FLOAT
   u64 sqlite3FloatSwap(u64 in);
 # define swapMixedEndianFloat(X)  X = sqlite3FloatSwap(X)
 #else
 # define swapMixedEndianFloat(X)
 #endif
-void sqlite3VdbeSerialGet(const unsigned char*, tSerial, Mem*);
+void sqlite3VdbeSerialGet(const unsigned char*, u32, Mem*);
 void sqlite3VdbeDeleteAuxData(sqlite3*, AuxData**, int, int);
 
 int sqlite2BtreeKeyCompare(BtCursor *, const void *, int, int, int *);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`sqlite2BtreeKeyCompare`
- 外部函数：`sqlite3FloatSwap`
- 外部函数：`sqlite3VdbeCursorRestore`
- 外部函数：`sqlite3VdbeDeleteAuxData`
- 外部函数：`sqlite3VdbeFinishMoveto`
- 外部函数：`sqlite3VdbeHandleMovedCursor`
- 外部函数：`sqlite3VdbeOneByteSerialTypeLen`
- 外部函数：`sqlite3VdbeSerialGet`
- 外部函数：`sqlite3VdbeSerialTypeLen`
- 外部函数：`swapMixedEndianFloat`
- 外部函数：`void`
- 大写宏：`SQLITE_DEBUG`
- 大写宏：`SQLITE_MIXED_ENDIAN_64BIT_FLOAT`
- 大写宏：`SQLITE_NOINLINE`
- 外部类型：`AuxData`
- 外部类型：`BtCursor`
- 外部类型：`Destructor`
- 外部类型：`MEM_Blob`
- 外部类型：`MEM_Dyn`
- 外部类型：`MEM_Str`
- 外部类型：`Mem`
- 外部类型：`OP_MakeRecord`
- 外部类型：`ShallowCopy`
- 外部类型：`Size`
- 外部类型：`Space`
- 外部类型：`The`
- 外部类型：`Transient`
- 外部类型：`VdbeCursor`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-dd7c51ff11` → 本草稿移入 `cases/defect/auto-sqlite-dd7c51ff11/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
