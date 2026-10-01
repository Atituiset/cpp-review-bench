// AUTO-DRAFT from redis/redis PR #15230
}
  // <<< BUG ANCHOR
    printf("Total connected nodes: %llu\n", (unsigned long long)*connected_nodes);
    printf("All links are bi-directiona? %s\n", (*reciprocal_links)?"yes":"no");
    return 0;
}
