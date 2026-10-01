// AUTO-DRAFT from nginx/nginx PR #1052
uintptr_t
ngx_escape_uri(u_char *dst, u_char *src, size_t size, ngx_uint_t type)
{
    ngx_uint_t      n;  // <<< BUG ANCHOR
    uint32_t       *escape;
    static u_char   hex[] = "0123456789ABCDEF";

    /*

                    /* mail_auth is the same as memcached */

    static uint32_t  *map[] =
        { uri, args, uri_component, html, refresh, memcached, memcached };


    escape = map[type];

    if (dst == NULL) {


    while (size) {
        if (escape[*src >> 5] & (1U << (*src & 0x1f))) {
            *dst++ = '%';
            *dst++ = hex[*src >> 4];
            *dst++ = hex[*src & 0xf];
            src++;
