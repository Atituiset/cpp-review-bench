// AUTO-DRAFT from torvalds/linux PR #ff47652a4b66c067c765a7ad464d930b5a9367cc
	ret = file_update_time(file);
	if (ret < 0)
		goto out;
	if (iocb->ki_flags & IOCB_NOWAIT) {
		/* We could block if there are any pages in the range. */
		ret = -EAGAIN;
