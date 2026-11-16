# auto-sqlite-7d763467f7

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#8a24548dfbe71bc2714079e7549d1814002f5c44](https://github.com/sqlite/sqlite/commit/8a24548dfbe71bc2714079e7549d1814002f5c44) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-16 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 15 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #8a24548dfbe71bc2714079e7549d1814002f5c44 (https://github.com/sqlite/sqlite/commit/8a24548dfbe71bc2714079e7549d1814002f5c44)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 4855；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 8a24548dfbe71bc2714079e7549d1814002f5c44 Make most tunable parameters in the query planner adjustable using :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -4833,31 +4833,40 @@ int sqlite3_test_control(int op, ...){
       break;
     }
 
-#if defined(SQLITE_DEBUG) && !defined(SQLITE_OMIT_WSD)
+#if defined(SQLITE_DEBUG) || defined(SQLITE_ENABLE_TUNING)
     /* sqlite3_test_control(SQLITE_TESTCTRL_TUNE, id, *piValue)
     **
-    ** If "id" is an integer between 1 and SQLITE_NTUNE then set the value
-    ** of the id-th tuning parameter to *piValue.  If "id" is between -1
-    ** and -SQLITE_NTUNE, then write the current value of the (-id)-th
-    ** tuning parameter into *piValue.
+    ** If "id" is an integer between 1 and N (where N is the number of
+    ** tuning parameters) then set the value of the id-th tuning parameter
+    ** to *piValue.  If "id" is between -1 and -N, then write the current
+    ** value of the (-id)-th tuning parameter into *piValue.  If "id" is
+    ** zero, return the number of tuning parameters (N).
     **
-    ** Tuning parameters are for use during transient development builds,
+    ** Tuning parameters are for use during development builds,
     ** to help find the best values for constants in the query planner.
-    ** Access tuning parameters using the Tuning(ID) macro.  Set the
+    ** Access tuning parameters using the Tuning(D,ID) macro.  Set the
     ** parameters in the CLI using ".testctrl tune ID VALUE".
     **
-    ** Transient use only.  Tuning parameters should not be used in
-    ** checked-in code.
+    ** See source code comments
+    **
+    ** Tuning parameters apply across all database connections for a process.
+    **
+    ** This interface is not threadsafe.  Do not change tuning parameters
+    ** while database connections are running on other threads.
     */
     case SQLITE_TESTCTRL_TUNE: {
       int id = va_arg(ap, int);
       int *piValue = va_arg(ap, int*);
-      if( id>0 && id<=SQLITE_NTUNE ){
-        Tuning(id) = *piValue;
-      }else if( id<0 && id>=-SQLITE_NTUNE ){
-        *piValue = Tuning(-id);
-      }else{
+      int N = sqlite3Tuning[0];
+      if( id>N || id<(-N) ){
         rc = SQLITE_NOTFOUND;
+        break;
+      }
+      rc = SQLITE_OK;
+      if( id>0 ){
+        sqlite3Tuning[id] = *piValue;
+      }else if( id<=0 ){
+        *piValue = sqlite3Tuning[-id];
       }
       break;
     }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`defined`
- 外部函数：`sqlite3_test_control`
- 外部函数：`the`
- 外部函数：`va_arg`
- 大写宏：`CLI`
- 大写宏：`SQLITE_DEBUG`
- 大写宏：`SQLITE_NOTFOUND`
- 大写宏：`SQLITE_NTUNE`
- 大写宏：`SQLITE_OMIT_WSD`
- 大写宏：`SQLITE_TESTCTRL_TUNE`
- 大写宏：`VALUE`
- 外部类型：`Access`
- 外部类型：`If`
- 外部类型：`Set`
- 外部类型：`Transient`
- 外部类型：`Tuning`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-7d763467f7` → 本草稿移入 `cases/defect/auto-sqlite-7d763467f7/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
