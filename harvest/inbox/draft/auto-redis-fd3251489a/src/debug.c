// AUTO-DRAFT from redis/redis PR #14968
"    Show low level info about `key` and associated value.",
"DROP-CLUSTER-PACKET-FILTER <packet-type>",
"    Drop all packets that match the filtered type. Set to -1 allow all packets.",
"OOM",
"    Crash the server simulating an out-of-memory error.",
"PANIC",
    {
        server.skip_checksum_validation = atoi(c->argv[2]->ptr);
        addReply(c,shared.ok);
    } else if (!strcasecmp(c->argv[1]->ptr,"aof-flush-sleep") &&
               c->argc == 3)
    {
