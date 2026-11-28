// AUTO-DRAFT from torvalds/linux PR #7641bffc3721f0d1650e52935369ec7f7bf84126
  // <<< BUG ANCHOR
	ret = kernel_sendmsg(udc->ud.tcp_socket, &msg, iov,
			     1, txsize);
	if (ret != txsize) {
		usbip_event_add(&udc->ud, VUDC_EVENT_ERROR_TCP);
		if (ret >= 0)
			return -EPIPE;
		return ret;
	}
	kfree(unlink);

	return txsize;
}
