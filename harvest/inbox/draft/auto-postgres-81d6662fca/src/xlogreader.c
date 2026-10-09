// AUTO-DRAFT from postgres/postgres PR #412848dac37f2e4455e1505f83f0af0f3d28a67d
											 &state->reset_cb);
#endif
  // <<< BUG ANCHOR
	if (state->seg.ws_file != -1)
		state->routine.segment_close(state);

	if (state->decode_buffer && state->free_decode_buffer)
