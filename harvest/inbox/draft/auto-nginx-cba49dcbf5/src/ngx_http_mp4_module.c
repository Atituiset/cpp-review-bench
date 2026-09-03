// AUTO-DRAFT from nginx/nginx PR #1147
ngx_http_mp4_trak_t *trak);
static ngx_int_t ngx_http_mp4_crop_stts_data(ngx_http_mp4_file_t *mp4,
    ngx_http_mp4_trak_t *trak, ngx_uint_t start);
static uint32_t ngx_http_mp4_seek_key_frame(ngx_http_mp4_file_t *mp4,  // <<< BUG ANCHOR
    ngx_http_mp4_trak_t *trak, uint32_t start_sample);
static ngx_int_t ngx_http_mp4_read_stss_atom(ngx_http_mp4_file_t *mp4,
    uint64_t atom_data_size);
static ngx_int_t ngx_http_mp4_update_stss_atom(ngx_http_mp4_file_t *mp4,
found:

    if (start) {
        key_prefix = ngx_http_mp4_seek_key_frame(mp4, trak, start_sample);

        start_sample -= key_prefix;

}


static uint32_t
ngx_http_mp4_seek_key_frame(ngx_http_mp4_file_t *mp4, ngx_http_mp4_trak_t *trak,
    uint32_t start_sample)
{
    uint32_t              key_prefix, sample, *entry, *end;
    ngx_buf_t            *data;
    ngx_http_mp4_conf_t  *conf;

    conf = ngx_http_get_module_loc_conf(mp4->request, ngx_http_mp4_module);
    if (!conf->start_key_frame) {
        return 0;
    }

    data = trak->out[NGX_HTTP_MP4_STSS_DATA].buf;
    if (data == NULL) {
        return 0;
    }

    entry = (uint32_t *) data->pos;
    /* sync samples starts from 1 */
    start_sample++;

    key_prefix = 0;

    while (entry < end) {
        sample = ngx_mp4_get_32value(entry);
        if (sample > start_sample) {
            break;
        }

        key_prefix = start_sample - sample;
        entry++;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, mp4->file.log, 0,
                   "mp4 key frame prefix:%uD", key_prefix);

    return key_prefix;
}
