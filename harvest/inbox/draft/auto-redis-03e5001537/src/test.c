// AUTO-DRAFT from redis/redis PR #15917
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <limits.h>
#include <math.h>

#include "hiredis.h"
#include "async.h"
#include "adapters/poll.h"
/* …（同文件无关代码省略）… */
struct pushCounters {
    int nil;
    int str;
};
/* …（同文件无关代码省略）… */
#define test(_s) { printf("#%02d ", ++tests); printf(_s); }
#define test_cond(_c) if(_c) printf("\033[0;32mPASSED\033[0;0m\n"); else {printf("\033[0;31mFAILED\033[0;0m\n"); fails++;}
/* …（同文件无关代码省略）… */
static void test_reply_reader(void) {
    redisReader *reader;
    void *reply, *root;
    int ret;
    int i;

    test("Error handling in reply parser: ");
    reader = redisReaderCreate();
    redisReaderFeed(reader,(char*)"@foo\r\n",6);
    ret = redisReaderGetReply(reader,NULL);
    test_cond(ret == REDIS_ERR &&
              strcasecmp(reader->errstr,"Protocol error, got \"@\" as reply type byte") == 0);
    redisReaderFree(reader);

    /* when the reply already contains multiple items, they must be free'd
     * on an error. valgrind will bark when this doesn't happen. */
    test("Memory cleanup in reply parser: ");
    reader = redisReaderCreate();
    redisReaderFeed(reader,(char*)"*2\r\n",4);
    redisReaderFeed(reader,(char*)"$5\r\nhello\r\n",11);
    redisReaderFeed(reader,(char*)"@foo\r\n",6);
    ret = redisReaderGetReply(reader,NULL);
    test_cond(ret == REDIS_ERR &&
              strcasecmp(reader->errstr,"Protocol error, got \"@\" as reply type byte") == 0);
    redisReaderFree(reader);

    reader = redisReaderCreate();
    test("Can handle arbitrarily nested multi-bulks: ");
    for (i = 0; i < 128; i++) {
        redisReaderFeed(reader,(char*)"*1\r\n", 4);
    }
    redisReaderFeed(reader,(char*)"$6\r\nLOLWUT\r\n",12);
    ret = redisReaderGetReply(reader,&reply);
    root = reply; /* Keep track of the root reply */
    test_cond(ret == REDIS_OK &&
        ((redisReply*)reply)->type == REDIS_REPLY_ARRAY &&
        ((redisReply*)reply)->elements == 1);

    test("Can parse arbitrarily nested multi-bulks correctly: ");
    while(i--) {
        assert(reply != NULL && ((redisReply*)reply)->type == REDIS_REPLY_ARRAY);
        reply = ((redisReply*)reply)->element[0];
    }
    test_cond(((redisReply*)reply)->type == REDIS_REPLY_STRING &&
        !memcmp(((redisReply*)reply)->str, "LOLWUT", 6));
    freeReplyObject(root);
    redisReaderFree(reader);

    test("Correctly parses LLONG_MAX: ");
    reader = redisReaderCreate();
    redisReaderFeed(reader, ":9223372036854775807\r\n",22);
    ret = redisReaderGetReply(reader,&reply);
    test_cond(ret == REDIS_OK &&
            ((redisReply*)reply)->type == REDIS_REPLY_INTEGER &&
            ((redisReply*)reply)->integer == LLONG_MAX);
    freeReplyObject(reply);
    redisReaderFree(reader);

    test("Set error when > LLONG_MAX: ");
    reader = redisReaderCreate();
    redisReaderFeed(reader, ":9223372036854775808\r\n",22);
    ret = redisReaderGetReply(reader,&reply);
    test_cond(ret == REDIS_ERR &&
              strcasecmp(reader->errstr,"Bad integer value") == 0);
    freeReplyObject(reply);
    redisReaderFree(reader);

    test("Correctly parses LLONG_MIN: ");
    reader = redisReaderCreate();
    redisReaderFeed(reader, ":-9223372036854775808\r\n",23);
    ret = redisReaderGetReply(reader,&reply);
    test_cond(ret == REDIS_OK &&
            ((redisReply*)reply)->type == REDIS_REPLY_INTEGER &&
            ((redisReply*)reply)->integer == LLONG_MIN);
    freeReplyObject(reply);
    redisReaderFree(reader);

    test("Set error when < LLONG_MIN: ");
    reader = redisReaderCreate();
    redisReaderFeed(reader, ":-9223372036854775809\r\n",23);
    ret = redisReaderGetReply(reader,&reply);
    test_cond(ret == REDIS_ERR &&
              strcasecmp(reader->errstr,"Bad integer value") == 0);
    freeReplyObject(reply);
    redisReaderFree(reader);

    test("Set error when array < -1: ");
    reader = redisReaderCreate();
    redisReaderFeed(reader, "*-2\r\n+asdf\r\n",12);
    ret = redisReaderGetReply(reader,&reply);
    test_cond(ret == REDIS_ERR &&
              strcasecmp(reader->errstr,"Multi-bulk length out of range") == 0);
    freeReplyObject(reply);
    redisReaderFree(reader);

    test("Set error when bulk < -1: ");
    reader = redisReaderCreate();
    redisReaderFeed(reader, "$-2\r\nasdf\r\n",11);
    ret = redisReaderGetReply(reader,&reply);
    test_cond(ret == REDIS_ERR &&
              strcasecmp(reader->errstr,"Bulk string length out of range") == 0);
    freeReplyObject(reply);
    redisReaderFree(reader);

    test("Can configure maximum multi-bulk elements: ");
    reader = redisReaderCreate();
    reader->maxelements = 1024;
    redisReaderFeed(reader, "*1025\r\n", 7);
    ret = redisReaderGetReply(reader,&reply);
    test_cond(ret == REDIS_ERR &&
              strcasecmp(reader->errstr, "Multi-bulk length out of range") == 0);
    freeReplyObject(reply);
    redisReaderFree(reader);

    test("Multi-bulk never overflows regardless of maxelements: ");
    size_t bad_mbulk_len = (SIZE_MAX / sizeof(void *)) + 3;
    char bad_mbulk_reply[100];
    snprintf(bad_mbulk_reply, sizeof(bad_mbulk_reply), "*%llu\r\n+asdf\r\n",
        (unsigned long long) bad_mbulk_len);

    reader = redisReaderCreate();
    reader->maxelements = 0;    /* Don't rely on default limit */
    redisReaderFeed(reader, bad_mbulk_reply, strlen(bad_mbulk_reply));
    ret = redisReaderGetReply(reader,&reply);
    test_cond(ret == REDIS_ERR && strcasecmp(reader->errstr, "Out of memory") == 0);
    freeReplyObject(reply);
    redisReaderFree(reader);

#if LLONG_MAX > SIZE_MAX
    test("Set error when array > SIZE_MAX: ");
    reader = redisReaderCreate();
    redisReaderFeed(reader, "*9223372036854775807\r\n+asdf\r\n",29);
    ret = redisReaderGetReply(reader,&reply);
    test_cond(ret == REDIS_ERR &&
            strcasecmp(reader->errstr,"Multi-bulk length out of range") == 0);
    freeReplyO
