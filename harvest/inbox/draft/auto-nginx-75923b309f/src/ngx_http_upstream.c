// AUTO-DRAFT from nginx/nginx PR #1170
last = h->value.data + h->value.len;
  // <<< BUG ANCHOR
        if (*(last - 1) == '"') {
            last--;
        }
