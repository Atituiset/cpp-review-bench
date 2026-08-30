# auto-redis-e9cdebd77c

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15364 (https://github.com/redis/redis/pull/15364)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -11,8 +11,10 @@
  * atomicSet(var,value)  -- Set the atomic counter value
  * atomicGetWithSync(var,value)  -- 'atomicGet' with inter-thread synchronization
  * atomicSetWithSync(var,value)  -- 'atomicSet' with inter-thread synchronization
+ * atomicSetRelease(var,value)  -- 'atomicSet' with release ordering
  * atomicCompareExchange(type,var,expected_var,desired)  --  Compare and exchange (CAS) operation
- * 
+ * atomicExchangeAcquire(var,newvalue,oldvalue_var)  --  Atomically store newvalue and fetch the previous value (acquire)
+ *
  * Atomic operations on flags. 
  * Flag type can be int, long, long long or their unsigned counterparts.
  * The value of the flag can be 1 or 0.
@@ -111,10 +113,14 @@
 } while(0)
 #define atomicSetWithSync(var,value) \
     atomic_store_explicit(&var,value,memory_order_seq_cst)
+#define atomicSetRelease(var,value) \
+    atomic_store_explicit(&var,value,memory_order_release)
 #define atomicCompareExchange(type,var,expected_var,desired) \
     atomic_compare_exchange_weak_explicit(&var,&expected_var,desired,memory_order_relaxed,memory_order_relaxed)
 #define atomicFlagGetSet(var,oldvalue_var) \
     oldvalue_var = atomic_exchange_explicit(&var,1,memory_order_relaxed)
+#define atomicExchangeAcquire(var,newvalue,oldvalue_var) \
+    oldvalue_var = atomic_exchange_explicit(&var,newvalue,memory_order_acquire)
 #define REDIS_ATOMIC_API "c11-builtin"
 
 #elif !defined(__ATOMIC_VAR_FORCE_SYNC_MACROS) && \
@@ -138,10 +144,14 @@
 } while(0)
 #define atomicSetWithSync(var,value) \
     __atomic_store_n(&var,value,__ATOMIC_SEQ_CST)
+#define atomicSetRelease(var,value) \
+    __atomic_store_n(&var,value,__ATOMIC_RELEASE)
 #define atomicCompareExchange(type,var,expected_var,desired) \
     __atomic_compare_exchange_n(&var,&expected_var,desired,1,__ATOMIC_RELAXED,__ATOMIC_RELAXED)
 #define atomicFlagGetSet(var,oldvalue_var) \
     oldvalue_var = __atomic_exchange_n(&var,1,__ATOMIC_RELAXED)
+#define atomicExchangeAcquire(var,newvalue,oldvalue_var) \
+    oldvalue_var = __atomic_exchange_n(&var,newvalue,__ATOMIC_ACQUIRE)
 #define REDIS_ATOMIC_API "atomic-builtin"
 
 #elif defined(HAVE_ATOMIC)
@@ -169,6 +179,8 @@
     ANNOTATE_HAPPENS_BEFORE(&var);  \
     while(!__sync_bool_compare_and_swap(&var,var,value,__sync_synchronize)); \
 } while(0)
+/* __sync has no release-only store; full barrier is a safe superset. */
+#define atomicSetRelease(var,value) atomicSetWithSync(var,value)
 #define atomicCompareExchange(type,var,expected_var,desired) ({ \
     type _old = __sync_val_compare_and_swap(&var,expected_var,desired); \
     int _success = (_old == expected_var); \
@@ -177,6 +189,12 @@
 })
 #define atomicFlagGetSet(var,oldvalue_var) \
     oldvalue_var = __sync_val_compare_and_swap(&var,0,1)
+#define atomicExchangeAcquire(var,newvalue,oldvalue_var) do { \
+    ANNOTATE_HAPPENS_BEFORE(&var); \
+    do { oldvalue_var = var; } \
+    while (!__sync_bool_compare_and_swap(&var,oldvalue_var,newvalue)); \
+    ANNOTATE_HAPPENS_AFTER(&var); \
+} while(0)
 #define REDIS_ATOMIC_API "sync-builtin"
 
 #else
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-e9cdebd77c` → 本草稿移入 `cases/defect/auto-redis-e9cdebd77c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
