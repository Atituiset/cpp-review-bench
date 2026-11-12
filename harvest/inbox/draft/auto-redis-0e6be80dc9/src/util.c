// AUTO-DRAFT from redis/redis PR #16233
 * case of one or more "../" appearing at the start of "filename"
 * relative path. */
sds getAbsolutePath(char *filename) {
    char cwd[1024];  // <<< BUG ANCHOR
    sds abspath;
    sds relpath = sdsnew(filename);
