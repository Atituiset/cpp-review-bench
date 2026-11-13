// AUTO-DRAFT from torvalds/linux PR #62754aad50d0b30938bd8ce30b43474ce502a2af
	newsk = req->sk;
  // <<< BUG ANCHOR
	if (sk->sk_protocol == IPPROTO_TCP &&
	    tcp_rsk(req)->tfo_listener) {
		spin_lock_bh(&queue->fastopenq.lock);
		if (tcp_rsk(req)->tfo_listener) {
			/* We are still waiting for the final ACK from 3WHS
