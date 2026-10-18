# auto-sqlite-9a35d3fab8

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#94852686fec2ebcb353ad65aedc90443af2aae48](https://github.com/sqlite/sqlite/commit/94852686fec2ebcb353ad65aedc90443af2aae48) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-18 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 17 |
| 编译错误数（gcc syntax-only） | 29（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #94852686fec2ebcb353ad65aedc90443af2aae48 (https://github.com/sqlite/sqlite/commit/94852686fec2ebcb353ad65aedc90443af2aae48)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1037；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 94852686fec2ebcb353ad65aedc90443af2aae48 Merge recent branch-3.54 fixes into trunk. :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -992,12 +992,21 @@ static int idxCreateFromCons(
       do{
         int i;
         char *zFind;
-        for(i=0; zCols[i]; i++){
-          h += ((h<<3) + zCols[i]);
-        }
+
         sqlite3_free(zName);
-        zName = sqlite3_mprintf("%s_idx_%08x", zTable, h);
+        if( collisions<10 ){
+          for(i=0; zCols[i]; i++){
+            h += ((h<<3) + zCols[i]);
+          }
+          zName = sqlite3_mprintf("%s_idx_%08x", zTable, h);
+        }else{
+          /* After several hash attempts, generate a random name */
+          sqlite3_uint64 u;
+          sqlite3_randomness(sizeof(u), &u);
+          zName = sqlite3_mprintf("%s_idx_%llx", zTable, u);
+        }
         if( zName==0 ) break;
+         
         /* Is is unique among table, view and index names? */
         zFmt = "SELECT count(*) FROM sqlite_schema WHERE name=%Q"
           " AND type in ('index','table','view')";
@@ -1011,8 +1020,8 @@ static int idxCreateFromCons(
           break;
         }
         ++collisions;
-      }while( collisions<50 && zName!=0 );
-      if( collisions ){
+      }while( ALWAYS(collisions<500) && zName!=0 );
+      if( NEVER(collisions) ){
         /* This return means "Gave up trying to find a unique index name." */
         rc = SQLITE_BUSY_TIMEOUT;
       }else if( zName==0 ){
@@ -1034,9 +1043,9 @@ static int idxCreateFromCons(
             idxHashAdd(&rc, &p->hIdx, zName, zIdx);
           }
         }
-        sqlite3_free(zName);
         sqlite3_free(zIdx);
       }
+      sqlite3_free(zName);
     }
 
     sqlite3_free(zCols);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`count`
- 外部函数：`in`
- 外部函数：`memcmp`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_malloc64`
- 外部函数：`sqlite3_mprintf`
- 大写宏：`AND`
- 大写宏：`FROM`
- 大写宏：`SELECT`
- 大写宏：`SQLITE_BUSY_TIMEOUT`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_OK`
- 大写宏：`WHERE`
- 外部类型：`Gave`
- 外部类型：`IdxHash`
- 外部类型：`IdxHashEntry`
- 外部类型：`Is`
- 外部类型：`This`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-9a35d3fab8` → 本草稿移入 `cases/defect/auto-sqlite-9a35d3fab8/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
