// AUTO-DRAFT from redis/redis PR #15242
call(c,CMD_CALL_FULL);

            serverAssert((c->flags & CLIENT_BLOCKED) == 0);
        }

        /* Commands may alter argc/argv, restore mstate. */
