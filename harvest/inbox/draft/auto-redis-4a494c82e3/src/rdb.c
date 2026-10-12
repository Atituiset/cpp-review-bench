// AUTO-DRAFT from redis/redis PR #14789
dbExpand(db, db_size, 0);
            dbExpandExpires(db, expires_size, 0);
            should_expand_db = 0;
        }

        /* With metadata, type = RDB_OPCODE_KEY_META. Layout: [<META>,]<TYPE>,<KEY>,<VALUE> */
