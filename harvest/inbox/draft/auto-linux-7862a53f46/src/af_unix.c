// AUTO-DRAFT from torvalds/linux PR #60aba79a50127e92d8c4a663d35b855e52a9da01
		if (old == plabel) {
			rcu_assign_pointer(ctx->peer_lastupdate,
					   aa_get_label(plabel));
		} else if (aa_label_is_subset(plabel, old)) {
			rcu_assign_pointer(ctx->peer_lastupdate,
					   aa_get_label(plabel));
			rcu_assign_pointer(ctx->peer, aa_get_label(plabel));
/* …（同文件无关代码省略）… */
	spin_lock(&unix_sk(sk)->lock);
	old = rcu_dereference_protected(ctx->peer,
					lockdep_is_held(&unix_sk(sk)->lock));
	l = aa_label_merge(old, label, GFP_ATOMIC);  // <<< BUG ANCHOR
	if (l) {
		if (l != old) {
			rcu_assign_pointer(ctx->peer, l);
			aa_put_label(old);
		} else
			aa_put_label(l);
	}
	spin_unlock(&unix_sk(sk)->lock);
}
