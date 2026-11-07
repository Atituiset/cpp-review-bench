# auto-nginx-d4aca9aa05

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | nginx/nginx |
| 源 PR | [#1908](https://github.com/nginx/nginx/pull/1908) |
| 许可证 | BSD-2-Clause |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-07 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 5 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1908 (https://github.com/nginx/nginx/pull/1908)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1158；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 1908 GeoIP: rejected out-of-range coordinates from MMDB lookups. :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -35,6 +35,8 @@
 #define NGX_GEOIP_CITY_DMA_CODE        10
 #define NGX_GEOIP_CITY_AREA_CODE       11
 
+#define NGX_GEOIP_MAX_COORD            180
+
 
 typedef struct {
 #if (NGX_HAVE_GEOIP)
@@ -1155,15 +1157,23 @@ ngx_stream_geoip_float(MMDB_entry_s *entry, const char **path, double *value)
 
     case MMDB_DATA_TYPE_DOUBLE:
         *value = entry_data.double_value;
-        return NGX_OK;
+        break;
 
     case MMDB_DATA_TYPE_FLOAT:
         *value = entry_data.float_value;
-        return NGX_OK;
+        break;
 
     default:
         return NGX_DECLINED;
     }
+
+    /* the range of a coordinate in degrees; NaN fails the comparison */
+
+    if (!(*value >= -NGX_GEOIP_MAX_COORD && *value <= NGX_GEOIP_MAX_COORD)) {
+        return NGX_DECLINED;
+    }
+
+    return NGX_OK;
 }
 
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 大写宏：`MMDB_DATA_TYPE_DOUBLE`
- 大写宏：`MMDB_DATA_TYPE_FLOAT`
- 大写宏：`NGX_DECLINED`
- 大写宏：`NGX_HAVE_GEOIP`
- 大写宏：`NGX_OK`

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

1. 完成上面检查清单后评论 `/case accept auto-nginx-d4aca9aa05` → 本草稿移入 `cases/defect/auto-nginx-d4aca9aa05/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
