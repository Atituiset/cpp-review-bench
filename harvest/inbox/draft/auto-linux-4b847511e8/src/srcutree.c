// AUTO-DRAFT from torvalds/linux PR #4cc6bbaeb3afe9166380eccd8d166d2dd75c0278
		// Call srcu_barrier() before this cleanup_srcu_struct()
		// to avoid triggering this WARN_ON().
		if (WARN_ON(timer_delete_sync(&sdp->delay_work) &&
			    rcu_segcblist_n_cbs(&sdp->srcu_cblist)) &&  // <<< BUG ANCHOR
		    rcu_cpu_beenfullyonline(sdp->cpu))
			queue_work_on(sdp->cpu, rcu_gp_wq, &sdp->work);
		flush_work(&sdp->work);
