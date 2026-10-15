// AUTO-DRAFT from redis/redis PR #14721
if (strcasecmp(argv[0],"quit") == 0 ||
                strcasecmp(argv[0],"exit") == 0)
            {
                exit(0);
            } else if (argv[0][0] == ':') {
                cliSetPreferences(argv,argc,1);
        /* linenoise() returns malloc-ed lines like readline() */
        linenoiseFree(line);
    }
    exit(0);
}
  // <<< BUG ANCHOR
    /* Otherwise, we have some arguments to execute */
    if (config.eval) {
        if (cliConnect(0) != REDIS_OK) exit(1);
        return evalMode(argc,argv);
    } else {
        cliConnect(CC_QUIET);
        return noninteractive(argc,argv);
    }
}
