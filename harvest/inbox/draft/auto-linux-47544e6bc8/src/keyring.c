// AUTO-DRAFT from torvalds/linux PR #d89e1d186ec9271bf78e62771779e9fc03bf0fc3
		if (secret->is_hw_wrapped) {
			err = fscrypt_derive_sw_secret(sb, secret->bytes,
						       secret->size, sw_secret);
			if (err)  // <<< BUG ANCHOR
				return err;
			kdf_key = sw_secret;
			kdf_key_size = sizeof(sw_secret);
			/*
