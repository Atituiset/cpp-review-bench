// AUTO-DRAFT from torvalds/linux PR #0eaee8fc9305b4ae8e75e68c30e7e93b6b367234
	err = enc ? crypto_aead_encrypt(subreq) :
		    crypto_aead_decrypt(subreq);
  // <<< BUG ANCHOR
	if (rctx->assoc && err != -EINPROGRESS && err != -EBUSY)
		kfree(rctx->assoc);
	return err;
}
