// AUTO-DRAFT from torvalds/linux PR #b71ccf604fbdf78450d3b17b5d69365caba29061
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <string.h>
  // <<< BUG ANCHOR
void memstick_new_req(struct memstick_host *host)
{
	if (host->card) {
		host->retries = cmd_retries;
		reinit_completion(&host->card->mrq_complete);
		host->request(host);
	}
}
/* …（同文件无关代码省略）… */
void memstick_init_req(struct memstick_request *mrq, unsigned char tpc,
		       const void *buf, size_t length)
{
	mrq->tpc = tpc;
	if (tpc & 8)
		mrq->data_dir = WRITE;
	else
		mrq->data_dir = READ;

	mrq->data_len = length > sizeof(mrq->data) ? sizeof(mrq->data) : length;
	if (mrq->data_dir == WRITE)
		memcpy(mrq->data, buf, mrq->data_len);

	mrq->long_data = 0;

	if (tpc == MS_TPC_SET_CMD || tpc == MS_TPC_EX_SET_CMD)
		mrq->need_card_int = 1;
	else
		mrq->need_card_int = 0;
}
/* …（同文件无关代码省略）… */
static int h_memstick_read_dev_id(struct memstick_dev *card,
				  struct memstick_request **mrq)
{
	struct ms_id_register id_reg = {};

	if (!(*mrq)) {
		memstick_init_req(&card->current_mrq, MS_TPC_READ_REG, &id_reg,
				  sizeof(struct ms_id_register));
		*mrq = &card->current_mrq;
		return 0;
	}
	if (!(*mrq)->error) {
		memcpy(&id_reg, (*mrq)->data, sizeof(id_reg));
		card->id.match_flags = MEMSTICK_MATCH_ALL;
		card->id.type = id_reg.type;
		card->id.category = id_reg.category;
		card->id.class = id_reg.class;
		dev_dbg(&card->dev, "if_mode = %02x\n", id_reg.if_mode);
	}
	complete(&card->mrq_complete);
	return -EAGAIN;
}
/* …（同文件无关代码省略）… */
static int h_memstick_set_rw_addr(struct memstick_dev *card,
				  struct memstick_request **mrq)
{
	if (!(*mrq)) {
		memstick_init_req(&card->current_mrq, MS_TPC_SET_RW_REG_ADRS,
				  (char *)&card->reg_addr,
				  sizeof(card->reg_addr));
		*mrq = &card->current_mrq;
		return 0;
	} else {
		complete(&card->mrq_complete);
		return -EAGAIN;
	}
}
/* …（同文件无关代码省略）… */
{
	card->next_request = h_memstick_set_rw_addr;
	memstick_new_req(card->host);
	if (!wait_for_completion_timeout(&card->mrq_complete,
			msecs_to_jiffies(500)))
		card->current_mrq.error = -ETIMEDOUT;

	return card->current_mrq.error;
}
/* …（同文件无关代码省略）… */

		card->next_request = h_memstick_read_dev_id;
		memstick_new_req(host);
		if (!wait_for_completion_timeout(&card->mrq_complete,
				msecs_to_jiffies(500)))
			card->current_mrq.error = -ETIMEDOUT;

		if (card->current_mrq.error)
			goto err_out;
