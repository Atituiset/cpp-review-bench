// AUTO-DRAFT from nginx/nginx PR #1484
for (i = 0; i < v->len; i++) {
        if (v->data[i] == '$') {
            if (v->data[i + 1] >= '1' && v->data[i + 1] <= '9') {  // <<< BUG ANCHOR
                nc++;

            } else {
