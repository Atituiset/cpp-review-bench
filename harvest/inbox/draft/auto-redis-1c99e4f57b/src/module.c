// AUTO-DRAFT from redis/redis PR #15673
RedisModuleString **argv;
    int argv_len;
    int argc;
    client *c;
} RedisModuleCommandFilterCtx;

        .argv = c->argv,
        .argv_len = c->argv_len,
        .argc = c->argc,
        .c = c
    };

        f->callback(&filter);
    }

    /* If the filter sets a new command, including command or subcommand,
     * the command looked up will be invalid. */
    c->lookedcmd = NULL;
    c->argv_len = filter.argv_len;
    c->argc = filter.argc;

    /* Update pending command if it exists. */
    pendingCommand *pcmd = c->current_pending_cmd;
    if (pcmd) {
        pcmd->argv = filter.argv;
        pcmd->argc = filter.argc;
        pcmd->argv_len = filter.argv_len;
        pcmd->cmd = NULL;
        pcmd->slot = INVALID_CLUSTER_SLOT;
        pcmd->flags = 0;

        /* Reset keys result */
        getKeysFreeResult(&pcmd->keys_result);
        pcmd->keys_result = (getKeysResult)GETKEYS_RESULT_INIT;
    }
}

    }
    fctx->argv[pos] = arg;
    fctx->argc++;

    return REDISMODULE_OK;
}

    decrRefCount(fctx->argv[pos]);
    fctx->argv[pos] = arg;

    return REDISMODULE_OK;
}
        fctx->argv[i] = fctx->argv[i+1];
    }
    fctx->argc--;

    return REDISMODULE_OK;
}
