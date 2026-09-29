// AUTO-DRAFT from curl/curl PR #10917
char creds[MAX_CREDS_LENGTH];
};
  // <<< BUG ANCHOR
OM_uint32 gss_init_sec_context(OM_uint32 *min,
            gss_const_cred_id_t initiator_cred_handle,
            gss_ctx_id_t *context_handle,
    return GSS_S_FAILURE;
  }

  name = strndup(input_name_buffer->value, input_name_buffer->length);
  if(!name) {
    *min = GSS_NO_MEMORY;
    return GSS_S_FAILURE;
