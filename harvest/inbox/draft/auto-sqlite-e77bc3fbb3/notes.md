# auto-sqlite-e77bc3fbb3

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#0290d4b7778eb75714d5cb97eefd14b5fd1f4096](https://github.com/sqlite/sqlite/commit/0290d4b7778eb75714d5cb97eefd14b5fd1f4096) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-04 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 17 |
| 编译错误数（gcc syntax-only） | 5（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #0290d4b7778eb75714d5cb97eefd14b5fd1f4096 (https://github.com/sqlite/sqlite/commit/0290d4b7778eb75714d5cb97eefd14b5fd1f4096)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 415；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 0290d4b7778eb75714d5cb97eefd14b5fd1f4096 Fix foreign key processing for the case where a row is its own parent and the parent key uses a non-binary collation. Re :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -398,7 +398,9 @@ static void fkLookupParent(
                sqlite3TableColumnToStorage(pFKey->pFrom, aiCol[i])+1+regData,
                regTemp+i);
       }
-  
+      sqlite3VdbeAddOp4(v, OP_Affinity, regTemp, nCol, 0,
+                        sqlite3IndexAffinityStr(pParse->db,pIdx), nCol);
+
       /* If the parent table is the same as the child table, and we are about
       ** to increment the constraint-counter (i.e. this is an INSERT operation),
       ** then check if the row being inserted matches itself. If so, do not
@@ -412,8 +414,8 @@ static void fkLookupParent(
       if( pTab==pFKey->pFrom && nIncr==1 ){
         int iJump = sqlite3VdbeCurrentAddr(v) + nCol + 1;
         for(i=0; i<nCol; i++){
-          int iChild = sqlite3TableColumnToStorage(pFKey->pFrom,aiCol[i])
-                              +1+regData;
+          CollSeq *pColl = sqlite3LocateCollSeq(pParse, pIdx->azColl[i]);
+          int iChild = regTemp+i;
           int iParent = 1+regData;
           iParent += sqlite3TableColumnToStorage(pIdx->pTable,
                                                  pIdx->aiColumn[i]);
@@ -423,14 +425,14 @@ static void fkLookupParent(
             /* The parent key is a composite key that includes the IPK column */
             iParent = regData;
           }
-          sqlite3VdbeAddOp3(v, OP_Ne, iChild, iJump, iParent); VdbeCoverage(v);
+          sqlite3VdbeAddOp4(v, OP_Ne, iChild, iJump, iParent,
+                            (char*)pColl, P4_COLLSEQ);
+          VdbeCoverage(v);
           sqlite3VdbeChangeP5(v, SQLITE_JUMPIFNULL);
         }
         sqlite3VdbeGoto(v, iOk);
       }
 
-      sqlite3VdbeAddOp4(v, OP_Affinity, regTemp, nCol, 0,
-                        sqlite3IndexAffinityStr(pParse->db,pIdx), nCol);
       sqlite3VdbeAddOp4Int(v, OP_Found, iCur, iOk, regTemp, nCol);
       VdbeCoverage(v);
       sqlite3ReleaseTempRange(pParse, regTemp, nCol);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`sqlite3IndexAffinityStr`
- 外部函数：`sqlite3ReleaseTempRange`
- 外部函数：`sqlite3TableColumnToStorage`
- 外部函数：`sqlite3VdbeAddOp3`
- 外部函数：`sqlite3VdbeAddOp4`
- 外部函数：`sqlite3VdbeAddOp4Int`
- 外部函数：`sqlite3VdbeChangeP5`
- 外部函数：`sqlite3VdbeCurrentAddr`
- 外部函数：`sqlite3VdbeGoto`
- 大写宏：`INSERT`
- 大写宏：`IPK`
- 大写宏：`SQLITE_JUMPIFNULL`
- 外部类型：`If`
- 外部类型：`OP_Affinity`
- 外部类型：`OP_Found`
- 外部类型：`OP_Ne`
- 外部类型：`The`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-e77bc3fbb3` → 本草稿移入 `cases/defect/auto-sqlite-e77bc3fbb3/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
