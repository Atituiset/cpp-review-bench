// AUTO-DRAFT from torvalds/linux PR #7c24a4e87e219072e5e106f6791146cbf3b056b3
{
	struct vsock_sock *vsk = vsock_sk(sk);

	/* Flush MSG_ZEROCOPY leftovers. */
	__skb_queue_purge(&sk->sk_error_queue);
