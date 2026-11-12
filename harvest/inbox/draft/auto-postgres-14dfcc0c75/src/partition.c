// AUTO-DRAFT from postgres/postgres PR #9a849767eb281c9502aab4a7384a654004ec6ffe
 * The first element is the immediate parent and the last one is the topmost
 * parent in the partition hierarchy.
 *
 * Note: Because this function assumes that the relation whose OID is passed
 * as an argument and each ancestor will have precisely one parent, it should
 * only be called when it is known that the relation is a partition.
