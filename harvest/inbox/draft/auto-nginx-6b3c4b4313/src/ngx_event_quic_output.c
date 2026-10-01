// AUTO-DRAFT from nginx/nginx PR #479
ngx_memzero(msg_control, sizeof(msg_control));
  // <<< BUG ANCHOR
    iov.iov_len = len;
    iov.iov_base = buf;

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    ngx_memzero(&msg, sizeof(struct msghdr));

    iov.iov_len = len;
    iov.iov_base = buf;

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
