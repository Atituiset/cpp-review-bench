// AUTO-DRAFT from redis/redis PR #15539
hashTemplateDefrag(tmpl);

        if (++iterations > 64) {
            iterations = 0;
            if (getMonotonicUs() >= endtime) return DEFRAG_NOT_DONE;
        }
 * actions. This interface allows defrag to continue running, avoiding a single long defrag step
 * after the long operation completes. */
void defragWhileBlocked(void) {
    /* This is called infrequently, while timers are not active. We might need to start defrag. */
    if (!defragIsRunning()) activeDefragCycle();
