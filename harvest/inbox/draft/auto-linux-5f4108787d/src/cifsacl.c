// AUTO-DRAFT from torvalds/linux PR #f1bfe4003916a6d56e43b08698f38bad4443bcf4
		goto out_revert_creds;
	} else if (sidkey->datalen < CIFS_SID_BASE_SIZE) {
		rc = smb_EIO1(smb_eio_trace_malformed_sid_key, sidkey->datalen);
		cifs_dbg(FYI, "%s: Downcall contained malformed key (datalen=%hu)\n",  // <<< BUG ANCHOR
			 __func__, sidkey->datalen);
		goto invalidate_key;
	}
/* …（同文件无关代码省略）… */
	if (ksid_size > sidkey->datalen) {
		rc = smb_EIO2(smb_eio_trace_malformed_ksid_key,
			      ksid_size, sidkey->datalen);
		cifs_dbg(FYI, "%s: Downcall contained malformed key (datalen=%hu, ksid_size=%u)\n",
			 __func__, sidkey->datalen, ksid_size);
		goto invalidate_key;
	}
/* …（同文件无关代码省略）… */
	 */
	BUILD_BUG_ON(sizeof(uid_t) != sizeof(gid_t));
	if (sidkey->datalen != sizeof(uid_t)) {
		cifs_dbg(FYI, "%s: Downcall contained malformed key (datalen=%hu)\n",
			 __func__, sidkey->datalen);
		key_invalidate(sidkey);
		goto out_key_put;
