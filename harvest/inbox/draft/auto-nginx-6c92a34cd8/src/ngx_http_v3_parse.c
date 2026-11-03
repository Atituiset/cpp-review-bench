// AUTO-DRAFT from nginx/nginx PR #1701
  // <<< BUG ANCHOR
            st->literal.length = st->pint.value;
            if (st->literal.length == 0) {
                st->value.len = 0;
                goto done;
            }

/* …（同文件无关代码省略）… */

            st->literal.length = st->pint.value;
            if (st->literal.length == 0) {
                st->value.len = 0;
                goto done;
            }
