// AUTO-DRAFT from postgres/postgres PR #126976a8ffcfa47f40d3df09243ebf4606001912
 * If an input tuple has a null join key, then it cannot match anything from
 * the other side of the join.  Normally we can just discard such a tuple
 * immediately, but if it comes from the outer side of an outer join then we
 * must emit it with null-extension of the other side.  For various reasons
 * it's not convenient to do that immediately on seeing the tuple, so we dump
 * the tuple into a tuplestore and emit it later.  (In the unlikely but
 * supported case of a non-strict join operator, we treat null keys as normal
 * data.)
 * ----------------------------------------------------------------
 */
