// AUTO-DRAFT from redis/redis PR #15255
} else if ((!strcmp(argv[i],"-a") || !strcmp(argv[i],"--pass"))
                   && !lastarg)
        {
            config.conn_info.auth = sdsnew(argv[++i]);
        } else if (!strcmp(argv[i],"--user") && !lastarg) {
            config.conn_info.user = sdsnew(argv[++i]);
        } else if (!strcmp(argv[i],"-u") && !lastarg) {
            parseRedisUri(argv[++i],"redis-cli",&config.conn_info,&config.tls);
