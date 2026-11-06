// AUTO-DRAFT from torvalds/linux PR #f66469d9905b39d2c260a15d7d6db450f4db67c1
	err = -EINVAL;
  // <<< BUG ANCHOR
	while (fw_ptr - fw->data < fw->size) {
		struct hci_command_hdr *cmd = (void *)(fw_ptr + frag_len);

		frag_len += sizeof(*cmd) + cmd->plen;
