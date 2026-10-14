// AUTO-DRAFT from redis/redis PR #15869
void pauseIOThreadsRange(int start, int end) {
    if (!server.io_threads_active) return;
    serverAssert(start >= 1 && end < server.io_threads_num && start <= end);
    serverAssert(pthread_equal(pthread_self(), server.main_thread_id));  // <<< BUG ANCHOR

    /* Try to make all io threads paused in parallel */
    for (int i = start; i <= end; i++) {
/* …（同文件无关代码省略）… */
void resumeIOThreadsRange(int start, int end) {
    if (!server.io_threads_active) return;
    serverAssert(start >= 1 && end < server.io_threads_num && start <= end);
    serverAssert(pthread_equal(pthread_self(), server.main_thread_id));

    for (int i = start; i <= end; i++) {
        serverAssert(PausedIOThreads[i] > 0);
/* …（同文件无关代码省略）… */
    }
}

/* Pause the specific io thread, and wait for it to be paused. */
void pauseIOThread(int id) {
    pauseIOThreadsRange(id, id);
