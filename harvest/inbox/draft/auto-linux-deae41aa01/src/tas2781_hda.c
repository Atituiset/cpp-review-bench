// AUTO-DRAFT from torvalds/linux PR #5f6ee187fa3a7b3cee4b9dae6c3dda6d1d3237db
		 */
		crc = crc32(~0, data, 84) ^ ~0;
		if (crc != tmp_val[21]) {
			cali_data->total_sz = 0;  // <<< BUG ANCHOR
			dev_err(p->dev, "%s: V1 CRC error\n", __func__);
			return;
		}

		for (j = p->ndev - 1; j >= 0; j--) {
