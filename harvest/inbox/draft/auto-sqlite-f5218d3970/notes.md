# auto-sqlite-f5218d3970

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#8c7ca7db873fb042d3fcb8c5e398039d30eff1e2](https://github.com/sqlite/sqlite/commit/8c7ca7db873fb042d3fcb8c5e398039d30eff1e2) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-13 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 64 |
| 编译错误数（gcc syntax-only） | 18（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #8c7ca7db873fb042d3fcb8c5e398039d30eff1e2 (https://github.com/sqlite/sqlite/commit/8c7ca7db873fb042d3fcb8c5e398039d30eff1e2)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 2213；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 8c7ca7db873fb042d3fcb8c5e398039d30eff1e2 Avoid integer overflows when calculating DESC RANGE windows. Report [bugs:/info/2026-11-04T20:35:05Z | 2026-11-04T20:35: :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2113,13 +2113,17 @@ static void windowIfNewPeer(
 ** comparison is reversed. Instead of adding regVal to csr1.peerVal, it is
 ** subtracted. And the comparison operator is inverted to - ">=" becomes "<=",
 ** ">" becomes "<", and so on. So, with DESC sort order, if the argument op
-** is OP_Ge, the generated code is equivalent to:
+** is OP_Ge, the test is logically:
 **
 **   if( csr1.peerVal - regVal <= csr2.peerVal ) goto lbl;
 **
-** A special type of arithmetic is used such that if csr1.peerVal is not
-** a numeric type (real or integer), then the result of the addition
-** or subtraction is a a copy of csr1.peerVal.
+** But in the DESC case, to avoid integer overflow in some cases, in practice:
+**
+**   if( csr1.peerVal <= csr2.peerVal + regVal ) goto lbl;
+**
+** A special type of arithmetic is used such that if the peer-value that
+** regVal is added to is not a numeric type (real or integer), then the
+** result of the addition is a copy of that peer-value.
 */
 static void windowCodeRangeTest(
   WindowCodeArg *p,
@@ -2135,7 +2139,8 @@ static void windowCodeRangeTest(
   int reg1 = sqlite3GetTempReg(pParse);     /* Reg. for csr1.peerVal+regVal */
   int reg2 = sqlite3GetTempReg(pParse);     /* Reg. for csr2.peerVal */
   int regString = ++pParse->nMem;           /* Reg. for constant value '' */
-  int arith = OP_Add;                       /* OP_Add or OP_Subtract */
+  int regArith = reg1;                      /* Register to add regVal to */
+  int bDesc = 0;                            /* True for DESC ORDER BY */
   int addrGe;                               /* Jump destination */
   int addrDone = sqlite3VdbeMakeLabel(pParse);   /* Address past OP_Ge */
   CollSeq *pColl;
@@ -2152,12 +2157,14 @@ static void windowCodeRangeTest(
       case OP_Gt: op = OP_Lt; break;
       default: assert( op==OP_Le ); op = OP_Ge; break;
     }
-    arith = OP_Subtract;
+    bDesc = 1;
+    regArith = reg2;
   }
 
-  VdbeModuleComment((v, "CodeRangeTest: if( R%d %s R%d %s R%d ) goto lbl",
-      reg1, (arith==OP_Add ? "+" : "-"), regVal,
-      ((op==OP_Ge) ? ">=" : (op==OP_Le) ? "<=" : (op==OP_Gt) ? ">" : "<"), reg2
+  VdbeModuleComment((v, "CodeRangeTest: if( R%d%s%s %s R%d%s%s ) goto lbl",
+      reg1, (bDesc ? "" : " + R"), (bDesc ? "" : "regVal"),
+      ((op==OP_Ge) ? ">=" : (op==OP_Le) ? "<=" : (op==OP_Gt) ? ">" : "<"), reg2,
+      (bDesc ? " + R" : ""), (bDesc ? "regVal" : "")
   ));
 
   /* If the BIGNULL flag is set for the ORDER BY, then it is required to
@@ -2205,26 +2212,27 @@ static void windowCodeRangeTest(
     VdbeCoverage(v);
   }
 
-  /* Register reg1 currently contains csr1.peerVal (the peer-value from csr1).
-  ** This block adds (or subtracts for DESC) the numeric value in regVal
-  ** from it. Or, if reg1 is not numeric (it is a NULL, a text value or a blob),
-  ** then leave reg1 as it is. In pseudo-code, this is implemented as:
+  /* Register regArith currently contains a peer-value - csr1.peerVal for
+  ** ASC, or csr2.peerVal for DESC. This block adds the numeric value in
+  ** regVal to it. Or, if regArith is not numeric (it is a NULL, a text value
+  ** or a blob), then leave it as it is. In pseudo-code, this is implemented
+  ** as:
   **
-  **   if( reg1>='' ) goto addrGe;
-  **   reg1 = reg1 +/- regVal
+  **   if( regArith>='' ) goto addrGe;
+  **   regArith = regArith + regVal
   **   addrGe:
   **
   ** Since all strings and blobs are greater-than-or-equal-to an empty string,
-  ** the add/subtract is skipped for these, as required. If reg1 is a NULL,
-  ** then the arithmetic is performed, but since adding or subtracting from
-  ** NULL is always NULL anyway, this case is handled as required too.  */
+  ** the addition is skipped for these, as required. If regArith is a NULL,
+  ** then the arithmetic is performed, but since adding to NULL is always
+  ** NULL anyway, this case is handled as required too.  */
   sqlite3VdbeAddOp4(v, OP_String8, 0, regString, 0, "", P4_STATIC);
-  addrGe = sqli
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`adds`
- 外部函数：`assert`
- 外部函数：`numeric`
- 外部函数：`peerVal`
- 外部函数：`sqlite3ExprNNCollSeq`
- 外部函数：`sqlite3GetTempReg`
- 外部函数：`sqlite3GetVdbe`
- 外部函数：`sqlite3VdbeAddOp1`
- 外部函数：`sqlite3VdbeAddOp2`
- 外部函数：`sqlite3VdbeAddOp3`
- 外部函数：`sqlite3VdbeAddOp4`
- 外部函数：`sqlite3VdbeJumpHere`
- 外部函数：`sqlite3VdbeMakeLabel`
- 外部函数：`windowCodeRangeTest`
- 大写宏：`BIGNULL`
- 大写宏：`DESC`
- 大写宏：`KEYINFO_ORDER_BIGNULL`
- 大写宏：`KEYINFO_ORDER_DESC`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 大写宏：`ORDER`
- 大写宏：`P4_STATIC`
- 大写宏：`VDBE`
- 外部类型：`Additionally`
- 外部类型：`Address`
- 外部类型：`And`
- 外部类型：`CodeRangeTest`
- 外部类型：`CollSeq`
- 外部类型：`Compare`
- 外部类型：`Cursor`
- 外部类型：`ExprList`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Instead`
- 外部类型：`Jump`
- 外部类型：`Note`
- 外部类型：`OP_Add`
- 外部类型：`OP_Column`
- 外部类型：`OP_Ge`
- 外部类型：`OP_Goto`
- 外部类型：`OP_Gt`
- 外部类型：`OP_IsNull`
- 外部类型：`OP_Le`
- 外部类型：`OP_Lt`
- 外部类型：`OP_NotNull`
- 外部类型：`OP_String8`
- 外部类型：`OP_Subtract`
- 外部类型：`Or`
- 外部类型：`Parse`
- 外部类型：`Read`
- 外部类型：`Reg`
- 外部类型：`Register`
- 外部类型：`Since`
- 外部类型：`So`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Vdbe`
- 外部类型：`Window`
- 外部类型：`WindowCodeArg`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-f5218d3970` → 本草稿移入 `cases/defect/auto-sqlite-f5218d3970/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
