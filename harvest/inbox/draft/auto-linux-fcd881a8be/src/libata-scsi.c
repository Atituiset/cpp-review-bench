// AUTO-DRAFT from torvalds/linux PR #fd179f8a05be3ccae366b9b96e176b51fbe54aab
  // <<< BUG ANCHOR
		/* descriptor format */
		len = sb[7];
		desc = (char *)scsi_sense_desc_find(sb, len + 8, 9);
		if (!desc) {
			if (SCSI_SENSE_BUFFERSIZE < len + 14)
				return;
			sb[7] = len + 14;
			desc = sb + 8 + len;
		}
		desc[0] = 9;
		desc[1] = 12;
