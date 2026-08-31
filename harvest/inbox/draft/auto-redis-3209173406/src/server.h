// AUTO-DRAFT from redis/redis PR #15604
typedef struct hashTemplateArray {
    uint64_t tmpl_id;    /* Template id; resolve via hashTemplateGetById. */
    unsigned long long field_count;
    sds values[];       /* Flexible array: values in template field order. */
} hashTemplateArray;
