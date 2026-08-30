// AUTO-DRAFT from nginx/nginx PR #702
return NULL;
        }
  // <<< BUG ANCHOR
        pkey = ENGINE_load_private_key(engine, (char *) last, 0, 0);

        if (pkey == NULL) {
            *err = "ENGINE_load_private_key() failed";
