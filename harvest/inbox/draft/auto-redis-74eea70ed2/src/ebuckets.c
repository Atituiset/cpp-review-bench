// AUTO-DRAFT from redis/redis PR #15950
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
typedef struct CommonSegHdr {
    eItem head;
} CommonSegHdr;
/* …（同文件无关代码省略）… */
typedef struct FirstSegHdr {
    eItem head;          /* first item in the list */
    uint32_t totalItems; /* total items in the bucket, across chained segments */
    uint32_t numSegs;    /* number of segments in the bucket */
} FirstSegHdr;
/* …（同文件无关代码省略）… */
typedef struct NextSegHdr {
    eItem head;
    CommonSegHdr *prevSeg; /* pointer to previous segment */
    FirstSegHdr *firstSeg; /* pointer to first segment of the bucket */
} NextSegHdr;
/* …（同文件无关代码省略）… */
void ebDefragRaxBucket(EbucketsType *type, raxIterator *ri,
                       ebDefragFunctions *defragfns, void *privdata)
{
    CommonSegHdr *currentSegHdr = ri->data;
    CommonSegHdr *firstSegHdr = currentSegHdr;
    eItem iter = ((FirstSegHdr*)currentSegHdr)->head;
    ExpireMeta *mHead = type->getExpireMeta(iter);
    ExpireMeta *prevSegLastItem = NULL; /* The last item of the previous segment */

    while (1) {
        unsigned int numItems = mHead->numItems;
        assert(numItems);  /* Avoid compiler warning with old build chain. */
        ExpireMeta *prevIter = NULL;
        ExpireMeta *mIter = NULL;

        for (unsigned int i = 0; i < numItems; ++i) {
            eItem newiter = defragfns->defragItem(iter, privdata);
            if (newiter) {
                iter = newiter;

                if (prevIter == NULL) {
                    /* If this is the first item in the segment, update the segment
                     * header to point to the new item location. */
                    currentSegHdr->head = iter;
                } else {
                    /* Update the previous item's next pointer to point to the newly defragmented item */
                    prevIter->next = iter;
                }
            }
            mIter = type->getExpireMeta(iter);
            prevIter = mIter;
            iter = mIter->next;
        }

        /* Try to defragment the current segment. */
        CommonSegHdr *newSegHdr = defragfns->defragAlloc(currentSegHdr);
        if (newSegHdr) {
            if (currentSegHdr == ri->data) {
                /* If it's the first segment, update the rax data pointer. */
                raxIteratorSetData(ri, newSegHdr);
                firstSegHdr = newSegHdr;
            } else {
                /* For non-first segments, update the previous segment's next
                 * item to new pointer. */
                prevSegLastItem->next = newSegHdr;
            }
            currentSegHdr = newSegHdr;
        }

        /* Remember last item in this segment for next iteration */
        prevSegLastItem = mIter;

        if (mIter->lastItemBucket) {
            /* The last eitem needs to point back to the segment. */
            if (newSegHdr) mIter->next = currentSegHdr;
            break;
        }

        NextSegHdr *nextSegHdr = mIter->next;
        nextSegHdr->firstSeg = (FirstSegHdr *)firstSegHdr;
        if (newSegHdr) {
            /* Update next segment's prev to point to the defragmented segment. */
            nextSegHdr->prevSeg = newSegHdr;
        }

        /* Update pointers for next segment iteration */
        iter = nextSegHdr->head;
        mHead = type->getExpireMeta(iter);
        currentSegHdr = (CommonSegHdr *)nextSegHdr;
    }
}
/* …（同文件无关代码省略）… */
    }

    /* Defrag the bucket in the rax node. */
    assert(raxNext(&ri));
    ebDefragRaxBucket(type, &ri, defragfns, privdata);

    /* Move to next node. */
/* …（同文件无关代码省略）… */
        }
    }

//    TEST("ebuckets - Create rax tree with extended-segment and add item before")

    return 0;
