// AUTO-DRAFT from torvalds/linux PR #bfddc420afe827d9f06458cdf8087cd602f8c9cb
		vif->num_queues = 0;
		synchronize_net();

		for (queue_index = 0; queue_index < num_queues; ++queue_index)
			xenvif_deinit_queue(&vif->queues[queue_index]);
