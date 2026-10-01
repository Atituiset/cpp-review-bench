// AUTO-DRAFT from nginx/nginx PR #1209
"mp4 time-to-sample entries:%uD", entries);
  // <<< BUG ANCHOR
    if (ngx_mp4_atom_data_size(ngx_mp4_stts_atom_t)
        + entries * sizeof(ngx_mp4_stts_entry_t) > atom_data_size)
    {
        ngx_log_error(NGX_LOG_ERR, mp4->file.log, 0,
                      "\"%s\" mp4 stts atom too small", mp4->file.name.data);
    atom->last = atom_table;

    if (ngx_mp4_atom_data_size(ngx_http_mp4_stss_atom_t)
        + entries * sizeof(uint32_t) > atom_data_size)
    {
        ngx_log_error(NGX_LOG_ERR, mp4->file.log, 0,
                      "\"%s\" mp4 stss atom too small", mp4->file.name.data);
    atom->last = atom_table;

    if (ngx_mp4_atom_data_size(ngx_mp4_ctts_atom_t)
        + entries * sizeof(ngx_mp4_ctts_entry_t) > atom_data_size)
    {
        ngx_log_error(NGX_LOG_ERR, mp4->file.log, 0,
                      "\"%s\" mp4 ctts atom too small", mp4->file.name.data);
                   "sample-to-chunk entries:%uD", entries);

    if (ngx_mp4_atom_data_size(ngx_mp4_stsc_atom_t)
        + entries * sizeof(ngx_mp4_stsc_entry_t) > atom_data_size)
    {
        ngx_log_error(NGX_LOG_ERR, mp4->file.log, 0,
                      "\"%s\" mp4 stsc atom too small", mp4->file.name.data);

    if (size == 0) {
        if (ngx_mp4_atom_data_size(ngx_mp4_stsz_atom_t)
            + entries * sizeof(uint32_t) > atom_data_size)
        {
            ngx_log_error(NGX_LOG_ERR, mp4->file.log, 0,
                          "\"%s\" mp4 stsz atom too small",
    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, mp4->file.log, 0, "chunks:%uD", entries);

    if (ngx_mp4_atom_data_size(ngx_mp4_stco_atom_t)
        + entries * sizeof(uint32_t) > atom_data_size)
    {
        ngx_log_error(NGX_LOG_ERR, mp4->file.log, 0,
                      "\"%s\" mp4 stco atom too small", mp4->file.name.data);
    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, mp4->file.log, 0, "chunks:%uD", entries);

    if (ngx_mp4_atom_data_size(ngx_mp4_co64_atom_t)
        + entries * sizeof(uint64_t) > atom_data_size)
    {
        ngx_log_error(NGX_LOG_ER
