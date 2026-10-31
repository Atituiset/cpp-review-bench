# auto-sqlite-a7a8c20d6a

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#49a32e906c18cc061752f88503019efb4a5c8ea0](https://github.com/sqlite/sqlite/commit/49a32e906c18cc061752f88503019efb4a5c8ea0) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-31 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 2 |
| 编译错误数（gcc syntax-only） | 16（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #49a32e906c18cc061752f88503019efb4a5c8ea0 (https://github.com/sqlite/sqlite/commit/49a32e906c18cc061752f88503019efb4a5c8ea0)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 804；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 49a32e906c18cc061752f88503019efb4a5c8ea0 When the zipfile extension writes a file with a timestamp before :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -786,6 +786,15 @@ static void zipfileMtimeToDos(ZipfileCDS *pCds, u32 mUnixTime){
   int yr, mon, day;
   int hr, min, sec;
 
+  if( mUnixTime<=315532800 ){
+    /* Experimentally, I find that Win11 will not report the modification
+    ** time for any ZIP file entry with a date before 1980-01-01 00:00:01.
+    ** So make that the minimum date-time value. */
+    pCds->mDate = 0x0021;
+    pCds->mTime = 0x0001;
+    return;
+  }
+
   A = (int)((JD - 1867216.25)/36524.25);
   A = (int)(JD + 1 + A - (A/4));
   B = A + 1524;
@@ -801,17 +810,11 @@ static void zipfileMtimeToDos(ZipfileCDS *pCds, u32 mUnixTime){
   min = (mUnixTime % (60*60)) / 60;
   sec = (mUnixTime % 60);
 
-  if( yr>=1980 ){
-    pCds->mDate = (u16)(day + (mon << 5) + ((yr-1980) << 9));
-    pCds->mTime = (u16)(sec/2 + (min<<5) + (hr<<11));
-  }else{
-    pCds->mDate = pCds->mTime = 0;
-  }
+  pCds->mDate = (u16)(day + (mon << 5) + ((yr-1980) << 9));
+  pCds->mTime = (u16)(sec/2 + (min<<5) + (hr<<11));
 
-  assert( mUnixTime<315507600 
-       || mUnixTime==zipfileMtime(pCds) 
+  assert( mUnixTime==zipfileMtime(pCds) 
        || ((mUnixTime % 2) && mUnixTime-1==zipfileMtime(pCds)) 
-       /* || (mUnixTime % 2) */
   );
 }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部类型：`JDsec`
- 外部类型：`ZipfileCDS`
- 外部类型：`uint16_t`
- 外部类型：`uint32_t`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-sqlite-a7a8c20d6a` → 本草稿移入 `cases/defect/auto-sqlite-a7a8c20d6a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
