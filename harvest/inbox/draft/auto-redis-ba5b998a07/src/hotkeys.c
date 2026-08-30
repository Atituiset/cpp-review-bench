// AUTO-DRAFT from redis/redis PR #15561
return;
                    }
  // <<< BUG ANCHOR
                    /* Check for duplicate slot */
                    for (int k = 0; k < i; k++) {
                        if (temp_slots[k] == slot_val) {
                            addReplyError(c, "duplicate slot number");
                            zfree(temp_slots);
                            return;
                        }
                    }

                    temp_slots[i] = (int)slot_val;
                }

                /* Sort the slots array */
                qsort(temp_slots, slots_count, sizeof(int), slotCompare);

                /* Build slotRangeArray from sorted slots */
                for (int i = 0; i < slots_count; i++) {
                    slots = slotRangeArrayAppend(slots, temp_slots[i]);
