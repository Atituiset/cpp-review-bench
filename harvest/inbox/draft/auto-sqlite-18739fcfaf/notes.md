# auto-sqlite-18739fcfaf

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#cb5e5b117eb4581d0bd32825e62e100bc61faa0b](https://github.com/sqlite/sqlite/commit/cb5e5b117eb4581d0bd32825e62e100bc61faa0b) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-07 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 3 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #cb5e5b117eb4581d0bd32825e62e100bc61faa0b (https://github.com/sqlite/sqlite/commit/cb5e5b117eb4581d0bd32825e62e100bc61faa0b)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 272；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT cb5e5b117eb4581d0bd32825e62e100bc61faa0b Fix an inaccuracy in the LIKE optimization for the corner case of a :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -269,9 +269,22 @@ static int isLikeOrGlob(
         cnt++;
       }else if( c>=0x80 ){
         const u8 *z2 = z+cnt-1;
-        if( c==0xff || sqlite3Utf8Read(&z2)==0xfffd  /* bad utf-8 */
-         || ENC(db)==SQLITE_UTF16LE 
+        u32 u;
+        if( c==0xff                             /* (1) */
+         || (u = sqlite3Utf8Read(&z2))>=0xfffd  /* (2) */
+         || u==0x7ff                            /* (3) */
+         || ENC(db)==SQLITE_UTF16LE             /* (4) */
         ){
+          /* Do not accept this character as part of the prefix because
+          **   (1)  0xff is not part of any valid UTF-8 character and the
+          **        0xff causes difficult when incrementing to the upper bound
+          **   (2)  0xfffd is not a valid UTF-8 character and values greater
+          **        start getting into the surrogate pairs in UTF16BE.
+          **   (3)  When incrementing this code point to the upper bound,
+          **        that would require adding an extra byte to the UTF-8
+          **   (4)  UTF16LE does not compare in sorted order.  Only
+          **        UTF16BE and UTF8 have that property.
+          */
           cnt--;
           break;
         }else{
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`sqlite3Utf8Read`
- 大写宏：`ENC`
- 大写宏：`SQLITE_UTF16LE`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-18739fcfaf` → 本草稿移入 `cases/defect/auto-sqlite-18739fcfaf/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
