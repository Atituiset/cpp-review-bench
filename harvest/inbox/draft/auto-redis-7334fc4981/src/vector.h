// AUTO-DRAFT from redis/redis PR #15065
size_t cap;        /* Capacity of the vector. */
    void **data;       /* Heap-allocated storage or refers to stack. */
    void **stack;      /* Optional stack buffer. */
} vec;

/* Return the contiguous backing array. */
#define vecData(v) ((v)->data)

/* Return the number of elements in the vector. */
#define vecSize(v) ((v)->size)

/* Initialize a vector */
void vecInit(vec *v, void **stack, size_t initcap);

/* Free only heap storage if any */
void vecRelease(vec *v);

/* Reset the logical length to zero while preserving allocated storage. */
