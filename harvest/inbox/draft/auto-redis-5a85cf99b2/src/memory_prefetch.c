// AUTO-DRAFT from redis/redis PR #15133
#include "server.h"
#include "dict.h"
  // <<< BUG ANCHOR
typedef enum { HT_IDX_FIRST = 0, HT_IDX_SECOND = 1, HT_IDX_INVALID = -1 } HashTableIndex;

typedef enum {
    PREFETCH_BUCKET,     /* Initial state, determines which hash table to use and prefetch the table's bucket */
    PREFETCH_ENTRY,      /* prefetch entries associated with the given key's hash */
    PREFETCH_KVOBJ,      /* prefetch the kv object of the entry found in the previous step */
    PREFETCH_VALDATA,    /* prefetch the value data of the kv object found in the previous step */
    PREFETCH_DONE        /* Indicates that prefetching for this key is complete */
} PrefetchState;


/************************************ State machine diagram for the prefetch operation. ********************************
                                                           │
                                                         start
                                                           │
                                    ┌────────────►└────────┬────────┘              │
                                    |                 Entry│found                  │
                                    │                      |                       │
                                    |              ┌───────▼────────┐              │
                                    │              | PREFETCH_KVOBJ |              ▼
                                    │              └───────┬────────┘              │
        kvobj not found - goto next entry                  |                       |
                                    │          ┌───────────▼────────────┐          │
                                    └──────◄───│    PREFETCH_VALDATA    │          ▼
                                               └───────────┬────────────┘          │
                                                           |                       │
                                                 ┌───────-─▼─────────────┐         │
                                    
