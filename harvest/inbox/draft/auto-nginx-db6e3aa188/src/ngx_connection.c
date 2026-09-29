// AUTO-DRAFT from nginx/nginx PR #1544
}
        }

#elif (NGX_HAVE_IP_DONTFRAG)

        if (ls[i].quic && ls[i].sockaddr->sa_family == AF_INET6) {
            value = 1;
