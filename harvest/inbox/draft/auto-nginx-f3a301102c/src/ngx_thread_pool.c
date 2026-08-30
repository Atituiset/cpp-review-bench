// AUTO-DRAFT from nginx/nginx PR #702
*lock = 0;
  // <<< BUG ANCHOR
    pthread_exit(0);
}
