# auto-redis-58efecc074

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | redis/redis |
| 源 PR | [#15604](https://github.com/redis/redis/pull/15604) |
| 许可证 | RSALv2 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-09-03 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 26 |
| 编译错误数（gcc syntax-only） | 24（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #15604 (https://github.com/redis/redis/pull/15604)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 2058；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 15604 Fix hashTypeAllocSize() for TMPL_ARRAY :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1159,6 +1159,7 @@ void hashTemplateLpFree(unsigned char *lp) {
 static hashTemplate *hashTemplateArrayGetTemplate(hashTemplateArray *hta) {
     hashTemplate *tmpl = hashTemplateGetById(hta->tmpl_id);
     serverAssert(tmpl != NULL);
+    serverAssert(tmpl->field_count == hta->field_count);
     return tmpl;
 }
 
@@ -1167,20 +1168,40 @@ static hashTemplate *hashTemplateArrayGetTemplate(hashTemplateArray *hta) {
  * Otherwise copies them with sdsdup. */
 static hashTemplateArray *hashTemplateArrayCreate(hashTemplate *tmpl, sds *values, int take) {
     unsigned long long n = tmpl->field_count;
-    hashTemplateArray *hta = zmalloc(sizeof(*hta) + sizeof(sds) * n);
+    size_t usable;
+    hashTemplateArray *hta = zmalloc_usable(sizeof(*hta) + sizeof(sds) * n, &usable);
     hta->tmpl_id = tmpl->id;
     hta->field_count = n;
+    hta->alloc_size = usable;
 
-    for (unsigned long long i = 0; i < n; i++)
+    for (unsigned long long i = 0; i < n; i++) {
         hta->values[i] = take ? values[i] : sdsdup(values[i]);
+        hta->alloc_size += sdsAllocSize(hta->values[i]);
+    }
 
     hashTemplateIncrKeyRef(tmpl);
     return hta;
 }
 
+static hashTemplateArray *hashTemplateArrayResize(hashTemplateArray *hta, unsigned long long field_count) {
+    size_t old_usable, new_usable;
+    hta = zrealloc_usable(hta, sizeof(*hta) + sizeof(sds) * field_count,
+                          &new_usable, &old_usable);
+    hta->alloc_size += new_usable;
+    hta->alloc_size -= old_usable;
+    return hta;
+}
+
 /* Free a hashTemplateArray (release key ref and free data). May run in a BIO
  * lazyfree thread: uses the tmpl_id/field_count, never the registry. */
 void hashTemplateArrayFree(hashTemplateArray *hta) {
+#ifdef DEBUG_ASSERTIONS
+    size_t values_size = 0;
+    for (unsigned long long i = 0; i < hta->field_count; i++)
+        values_size += sdsAllocSize(hta->values[i]);
+    debugServerAssert(hta->alloc_size - values_size == zmalloc_usable_size(hta));
+#endif
+
     for (unsigned long long i = 0; i < hta->field_count; i++)
         sdsfree(hta->values[i]);
 
@@ -2055,13 +2076,15 @@ int hashTypeSet(redisDb *db, kvobj *o, sds field, sds value, int flags) {
                 serverAssert(o->ptr != NULL);
             } else {
                 hashTemplateArray *hta = o->ptr;
-                if (hta->values[field_idx]) sdsfree(hta->values[field_idx]);
+                hta->alloc_size -= sdsAllocSize(hta->values[field_idx]);
+                sdsfree(hta->values[field_idx]);
                 if (flags & HASH_SET_TAKE_VALUE) {
                     hta->values[field_idx] = value;  /* adopt, don't copy */
                     value = NULL;
                 } else {
                     hta->values[field_idx] = sdsdup(value);
                 }
+                hta->alloc_size += sdsAllocSize(hta->values[field_idx]);
             }
             update = 1;
             goto cleanup;
@@ -2090,7 +2113,7 @@ int hashTypeSet(redisDb *db, kvobj *o, sds field, sds value, int flags) {
         } else {
             hashTemplateArray *hta = o->ptr;
             /* Expand struct and shift elements to make room. */
-            hta = zrealloc(hta, sizeof(*hta) + sizeof(sds) * new_field_count);
+            hta = hashTemplateArrayResize(hta, new_field_count);
             if ((unsigned long long)insert_pos < tmpl->field_count) {
                 memmove(&hta->values[insert_pos + 1], &hta->values[insert_pos],
                         sizeof(sds) * (tmpl->field_count - insert_pos));
@@ -2101,6 +2124,7 @@ int hashTypeSet(redisDb *db, kvobj *o, sds field, sds value, int flags) {
             } else {
                 hta->values[insert_pos] = sdsdup(value);
             }
+            hta->alloc_size += sdsAllocSize(hta->values[insert_pos]);
             hashTemplateDecrKeyRef(tmpl);
             hta->tmpl_id = new_tmpl->id;
             hta->field_count = new_tmpl->field_count;
@@ -2418,12 +2442,13 @@ int hashTypeDelete(robj *o, void *field) {
       
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`createObject`
- 外部函数：`dictDelete`
- 外部函数：`hashTemplateArrayFree`
- 外部函数：`lpBytes`
- 外部函数：`memmove`
- 外部函数：`moduleThreadHoldsGIL`
- 外部函数：`pthread_equal`
- 外部函数：`pthread_self`
- 外部函数：`sdsAllocSize`
- 外部函数：`sdsdup`
- 外部函数：`sdsfree`
- 外部函数：`serverAssert`
- 外部函数：`serverPanic`
- 外部函数：`zmalloc`
- 外部函数：`zrealloc`
- 大写宏：`BIO`
- 大写宏：`NULL`
- 大写宏：`OBJ_ENCODING_TMPL_ARRAY`
- 大写宏：`OBJ_ENCODING_TMPL_LP`
- 大写宏：`OBJ_HASH`
- 大写宏：`TMPL_CHUNK`
- 外部类型：`Create`
- 外部类型：`Expand`
- 外部类型：`Free`
- 外部类型：`May`
- 外部类型：`Otherwise`
- 外部类型：`Unknown`
- 外部类型：`and`
- 外部类型：`size_t`
- 外部类型：`uint64_t`

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

1. 完成上面检查清单后评论 `/case accept auto-redis-58efecc074` → 本草稿移入 `cases/defect/auto-redis-58efecc074/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
