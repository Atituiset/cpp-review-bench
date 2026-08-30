// AUTO-DRAFT from redis/redis PR #14844
} else {
            /* BULK_STR_REF - expand to full RESP format */
            bulkStrRef *str_ref = (bulkStrRef *)(ptr + sizeof(payloadHeader));

            /* Append prefix: "$<len>\r\n" */
            ret += reqresAppendBuffer(c, str_ref->prefix, str_ref->prefix_cnt);
