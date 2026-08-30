// AUTO-DRAFT from redis/redis PR #15203
}  // <<< BUG ANCHOR

int isValidAuxChar(int c) {
    return isalnum(c) || (strchr("!#$%&()*+:;<>?@[]^{|}~", c) == NULL);
}

int isValidAuxString(char *s, unsigned int length) {
