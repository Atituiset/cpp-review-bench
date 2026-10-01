// AUTO-DRAFT from redis/redis PR #14932
/* Exit if result set is empty as any additional removal
                * of elements will have no effect. */
            if (cardinality == 0) break;  // <<< BUG ANCHOR
        }
        zuiClearIterator(&src[j]);
