// AUTO-DRAFT from nginx/nginx PR #1648
while (*(uintptr_t *) le.ip) {
  // <<< BUG ANCHOR
            lcode = *(ngx_http_script_len_code_pt *) le.ip;
            key_len = (u_char) lcode(&le);

            lcode = *(ngx_http_script_len_code_pt *) le.ip;
            skip_empty = lcode(&le);
