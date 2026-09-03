// AUTO-DRAFT from redis/redis PR #13773
}
    }

    /* Free the AOF manifest. */
    if (server.aof_manifest) aofManifestFree(server.aof_manifest);

            exit(1);
        if (ret != AOF_NOT_EXIST)
            serverLog(LL_NOTICE, "DB loaded from append only file: %.3f seconds", (float)(ustime()-start)/1000000);
    } else {
        rdbSaveInfo rsi = RDB_SAVE_INFO_INIT;
        int rsi_is_valid = 0;
