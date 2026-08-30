// AUTO-DRAFT from nginx/nginx PR #1161
break;
  // <<< BUG ANCHOR
        case sw_literal:
            if (ch >= '0' && ch <= '9') {
                s->literal_len = s->literal_len * 10 + (ch - '0');
                break;

        case sw_end_literal_argument:
            switch (ch) {
            case '{':
                if (s->args.nelts <= 2) {
                    state = sw_literal;
                    break;
                }
                goto invalid;
            case CR:
                state = sw_almost_done;
                break;
            case LF:
                goto done;
            default:
                state = sw_spaces_before_argument;
                break;
            }
            break;
