// AUTO-DRAFT from torvalds/linux PR #4375b45d7060a6a3f41e14a9181c5c913ea474a4
  // <<< BUG ANCHOR
	return 0;
out_free_sg:
	pci_p2pmem_free_sgl(req->p2p_dev, req->sg);
out_err:
	return -ENOMEM;
}
