// AUTO-DRAFT from torvalds/linux PR #fe4167bf53b61405f97614f644f6a63a5d6ad034
  // <<< BUG ANCHOR
	/* Success! Copy everything to peer */
	down_write(&handshake->lock);
	/* It's important to check that the state is still the same, while we
	 * have an exclusive lock.
	 */
	if (handshake->state != state) {
		up_write(&handshake->lock);
		goto fail;
	}
