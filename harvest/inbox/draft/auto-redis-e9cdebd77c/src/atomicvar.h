// AUTO-DRAFT from redis/redis PR #15364
* atomicSet(var,value)  -- Set the atomic counter value
 * atomicGetWithSync(var,value)  -- 'atomicGet' with inter-thread synchronization
 * atomicSetWithSync(var,value)  -- 'atomicSet' with inter-thread synchronization
 * atomicCompareExchange(type,var,expected_var,desired)  --  Compare and exchange (CAS) operation
 * 
 * Atomic operations on flags. 
 * Flag type can be int, long, long long or their unsigned counterparts.
 * The value of the flag can be 1 or 0.
} while(0)
#define atomicSetWithSync(var,value) \
    atomic_store_explicit(&var,value,memory_order_seq_cst)
#define atomicCompareExchange(type,var,expected_var,desired) \
    atomic_compare_exchange_weak_explicit(&var,&expected_var,desired,memory_order_relaxed,memory_order_relaxed)
#define atomicFlagGetSet(var,oldvalue_var) \
    oldvalue_var = atomic_exchange_explicit(&var,1,memory_order_relaxed)
#define REDIS_ATOMIC_API "c11-builtin"

#elif !defined(__ATOMIC_VAR_FORCE_SYNC_MACROS) && \
} while(0)
#define atomicSetWithSync(var,value) \
    __atomic_store_n(&var,value,__ATOMIC_SEQ_CST)
#define atomicCompareExchange(type,var,expected_var,desired) \
    __atomic_compare_exchange_n(&var,&expected_var,desired,1,__ATOMIC_RELAXED,__ATOMIC_RELAXED)
#define atomicFlagGetSet(var,oldvalue_var) \
    oldvalue_var = __atomic_exchange_n(&var,1,__ATOMIC_RELAXED)
#define REDIS_ATOMIC_API "atomic-builtin"

#elif defined(HAVE_ATOMIC)
    ANNOTATE_HAPPENS_BEFORE(&var);  \
    while(!__sync_bool_compare_and_swap(&var,var,value,__sync_synchronize)); \
} while(0)
#define atomicCompareExchange(type,var,expected_var,desired) ({ \
    type _old = __sync_val_compare_and_swap(&var,expected_var,desired); \
    int _success = (_old == expected_var); \
})
#define atomicFlagGetSet(var,oldvalue_var) \
    oldvalue_var = __sync_val_compare_and_swap(&var,0,1)
#define REDIS_ATOMIC_API "sync-builtin"

#else
