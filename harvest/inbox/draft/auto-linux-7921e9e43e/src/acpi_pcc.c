// AUTO-DRAFT from torvalds/linux PR #4df4d2a577db2d3d57b888bcfb1206fc74661d16
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
 * to PCC commands
 */
#define PCC_CMD_WAIT_RETRIES_NUM	500ULL

struct pcc_data {
	struct pcc_mbox_chan *pcc_chan;
/* …（同文件无关代码省略）… */
acpi_pcc_address_space_setup(acpi_handle region_handle, u32 function,
			     void *handler_context,  void **region_context)
{
	struct pcc_data *data;
	struct acpi_pcc_info *ctx = handler_context;
	struct pcc_mbox_chan *pcc_chan;
	acpi_status ret;

	data = kzalloc_obj(*data);
	if (!data)
/* …（同文件无关代码省略）… */
	}

	pcc_chan = data->pcc_chan;
	if (!pcc_chan->mchan->mbox->txdone_irq) {
		pr_err("This channel-%d does not support interrupt.\n",
		       ctx->subspace_id);
		ret = AE_SUPPORT;
		goto err_free_channel;
	}

	*region_context = data;
	return AE_OK;

/* …（同文件无关代码省略）… */
			       u32 bits, acpi_integer *value,
			       void *handler_context, void *region_context)
{
	int ret;
	struct pcc_data *data = region_context;
	u64 usecs_lat;

	reinit_completion(&data->done);

	/* Write to Shared Memory */
	memcpy_toio(data->pcc_chan->shmem, (void *)value, data->ctx.length);

	ret = mbox_send_message(data->pcc_chan->mchan, NULL);
	if (ret < 0)
		return AE_ERROR;

	/*
	 * pcc_chan->latency is just a Nominal value. In reality the remote
	 * processor could be much slower to reply. So add an arbitrary
	 * amount of wait on top of Nominal.
	 */
	usecs_lat = PCC_CMD_WAIT_RETRIES_NUM * data->pcc_chan->latency;
	ret = wait_for_completion_timeout(&data->done,
						usecs_to_jiffies(usecs_lat));
	if (ret == 0) {
		pr_err("PCC command executed timeout!\n");
		return AE_TIME;
	}

	mbox_chan_txdone(data->pcc_chan->mchan, ret);

	memcpy_fromio(value, data->pcc_chan->shmem, data->ctx.length);

	return AE_OK;
}
