// AUTO-DRAFT from redis/redis PR #15055
* - debug.c - xorObjectDigest, serverLogObjectDebugInfo
 * - defrag.c - defragKey
 * - module.c - RM_KeyType (and add the new keytype to redismodule.h)
 * - object.c - object(create/free/dismiss/allocSize/Length) */

/* Extract encver / signature from a module type ID. */
#define REDISMODULE_TYPE_ENCVER_BITS 10
