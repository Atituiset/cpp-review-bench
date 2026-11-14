// AUTO-DRAFT from torvalds/linux PR #2800868f63227c8237d70c41e70d441c919620d6
	status->trigger_tstamp_sec = runtime->trigger_tstamp.tv_sec;
	status->trigger_tstamp_nsec = runtime->trigger_tstamp.tv_nsec;
	if (snd_pcm_running(substream)) {
		snd_pcm_update_hw_ptr(substream);  // <<< BUG ANCHOR
		if (runtime->tstamp_mode == SNDRV_PCM_TSTAMP_ENABLE) {
			status->tstamp_sec = runtime->status->tstamp.tv_sec;
			status->tstamp_nsec =
