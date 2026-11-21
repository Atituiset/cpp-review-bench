// AUTO-DRAFT from torvalds/linux PR #c44104f3642da720164042623e4832836aefdd8e
			goto unlock_bo;
		}

		if (is_partial && pexec[i].resize > bo->base.size) {
			ret = -EINVAL;
			goto unlock_bo;
/* …（同文件无关代码省略）… */
	}

	bo = to_qaic_bo(obj);
	timeout = args->timeout ? args->timeout : wait_exec_default_timeout_ms;
	timeout = msecs_to_jiffies(timeout);
	ret = wait_for_completion_interruptible_timeout(&bo->xfer_done, timeout);
