// AUTO-DRAFT from torvalds/linux PR #af32da41b0327b9c6a37856ba82b6760d6c8d10e
#define BTINTEL_PCIE_HCI_TYPE_LEN	4
/* …（同文件无关代码省略）… */
  // <<< BUG ANCHOR
	tfd_index = data->ia.tr_hia[BTINTEL_PCIE_TXQ_NUM];

	if (tfd_index > txq->count)
		return -ERANGE;

	if (skb->len > BTINTEL_PCIE_BUFFER_SIZE - BTINTEL_PCIE_HCI_TYPE_LEN) {
/* …（同文件无关代码省略）… */
static int btintel_pcie_setup_irq(struct btintel_pcie_data *data)
{
	int err;
	int num_irqs, i;

	for (i = 0; i < BTINTEL_PCIE_MSIX_VEC_MAX; i++)
		data->msix_entries[i].entry = i;
/* …（同文件无关代码省略）… */
						KBUILD_MODNAME,
						msix_entry);
		if (err) {
			pci_free_irq_vectors(data->pdev);
			data->alloc_vecs = 0;
			return err;
/* …（同文件无关代码省略）… */
	if (IS_ERR(data->base_addr))
		return PTR_ERR(data->base_addr);

	err = btintel_pcie_setup_irq(data);
	if (err)
		return err;
/* …（同文件无关代码省略）… */
		struct msix_entry *msix_entry;

		msix_entry = &data->msix_entries[i];
		free_irq(msix_entry->vector, msix_entry);
	}

	pci_free_irq_vectors(pdev);
