// AUTO-DRAFT from torvalds/linux PR #f66469d9905b39d2c260a15d7d6db450f4db67c1
				tlv_nvm->data[0] |= 0x80;
  // <<< BUG ANCHOR
				/* UART Baud Rate */
				if (soc_type >= QCA_WCN3991)
					tlv_nvm->data[1] = nvm_baud_rate;
				else
					tlv_nvm->data[2] = nvm_baud_rate;
