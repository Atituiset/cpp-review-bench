// AUTO-DRAFT from curl/curl PR #11384
struct stream_ctx *stream = H2_STREAM_CTX(data);
  int rv = 0;
  // <<< BUG ANCHOR
  if((sweight_wanted(data) != sweight_in_effect(data)) ||
     (data->set.priority.exclusive != data->state.priority.exclusive) ||
     (data->set.priority.parent != data->state.priority.parent) ) {
    /* send new weight and/or dependency */
    nghttp2_priority_spec pri_spec;
