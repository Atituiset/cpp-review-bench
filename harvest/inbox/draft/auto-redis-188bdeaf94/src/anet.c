// AUTO-DRAFT from redis/redis PR #15537
flags & ANET_CONNECT_NONBLOCK)
            return s;

        anetSetError(err, "connect: %s", strerror(errno));
        close(s);
        return ANET_ERR;
    }
    return s;
}

static int anetListen(char *err, int s, struct sockaddr *sa, socklen_t len, int backlog, mode_t perm) {
    if (bind(s,sa,len) == -1) {
        anetSetError(err, "bind: %s", strerror(errno));
