// AUTO-DRAFT from sqlite/sqlite PR #5feff8c9b784fc80b266bcb8ddbab186e010eb6d
        int addrInTop;         /* Top of the IN loop */
        int iBase;             /* Base register of multi-key index record */
        int nPrefix;           /* Number of prior entries in the key */
        u8 eEndLoopOp;         /* IN Loop terminator. OP_Next or OP_Prev */
      } *aInLoop;           /* Information about each nested IN operator */
    } in;                 /* Used when pWLoop->wsFlags&WHERE_IN_ABLE */
