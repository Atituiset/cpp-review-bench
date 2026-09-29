// AUTO-DRAFT from nginx/nginx PR #1648
continue;
            }
  // <<< BUG ANCHOR
            params_len += 1 + key_len + ((val_len > 127) ? 4 : 1) + val_len;
        }

        len += params_len;
        while (*(uintptr_t *) le.ip) {

            lcode = *(ngx_http_script_len_code_pt *) le.ip;
            key_len = (u_char) lcode(&le);

            lcode = *(ngx_http_script_len_code_pt *) le.ip;
            skip_empty = lcode(&le);
                continue;
            }

            if (ngx_http_script_check_length(&e, 1 + ((val_len > 127) ? 4 : 1))
                != NGX_OK)
            {
                return NGX_ERROR;
            }

            *e.pos++ = (u_char) key_len;

            if (val_len > 127) {
                *e.pos++ = (u_char) (((val_len >> 24) & 0x7f) | 0x80);
