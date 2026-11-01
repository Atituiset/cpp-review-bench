// AUTO-DRAFT from torvalds/linux PR #74f6055d0c5997b13e6a5253fdc2263f3af90408
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <string.h>
  // <<< BUG ANCHOR
static int imx_rpmsg_insert_workqueue(struct snd_pcm_substream *substream,
				      struct rpmsg_msg *msg,
				      struct rpmsg_info *info)
{
	int ret = 0;

	/*
	 * Queue the work to workqueue.
	 * If the queue is full, drop the message.
	 */
	guard(spinlock_irqsave)(&info->wq_lock);
	if (info->work_write_index != info->work_read_index) {
		int index = info->work_write_index;

		memcpy(&info->work_list[index].msg, msg,
		       sizeof(struct rpmsg_s_msg));

		queue_work(info->rpmsg_wq, &info->work_list[index].work);
		info->work_write_index++;
		info->work_write_index %= WORK_MAX_NUM;
	} else {
		info->msg_drop_count[substream->stream]++;
		ret = -EPIPE;
	}

	return ret;
}
/* …（同文件无关代码省略）… */
static int imx_rpmsg_pcm_hw_params(struct snd_soc_component *component,
				   struct snd_pcm_substream *substream,
				   struct snd_pcm_hw_params *params)
{
	struct rpmsg_info *info = dev_get_drvdata(component->dev);
	struct rpmsg_msg *msg;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		msg = &info->msg[TX_HW_PARAM];
		msg->s_msg.header.cmd = TX_HW_PARAM;
	} else {
		msg = &info->msg[RX_HW_PARAM];
		msg->s_msg.header.cmd = RX_HW_PARAM;
	}

	msg->s_msg.param.rate = params_rate(params);

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		msg->s_msg.param.format   = RPMSG_S16_LE;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		msg->s_msg.param.format   = RPMSG_S24_LE;
		break;
	case SNDRV_PCM_FORMAT_DSD_U16_LE:
		msg->s_msg.param.format   = RPMSG_DSD_U16_LE;
		break;
	case SNDRV_PCM_FORMAT_DSD_U32_LE:
		msg->s_msg.param.format   = RPMSG_DSD_U32_LE;
		break;
	default:
		msg->s_msg.param.format   = RPMSG_S32_LE;
		break;
	}

	switch (params_channels(params)) {
	case 1:
		msg->s_msg.param.channels = RPMSG_CH_LEFT;
		break;
	case 2:
		msg->s_msg.param.channels = RPMSG_CH_STEREO;
		break;
	default:
		msg->s_msg.param.channels = params_channels(params);
		break;
	}

	info->send_message(msg, info);

	return 0;
}

static snd_pcm_uframes_t imx_rpmsg_pcm_pointer(struct snd_soc_component *component,
					       struct snd_pcm_substream *substream)
{
/* …（同文件无关代码省略）… */
static void imx_rpmsg_timer_callback(struct timer_list *t)
{
	struct stream_timer  *stream_timer =
			timer_container_of(stream_timer, t, timer);
	struct snd_pcm_substream *substream = stream_timer->substream;
	struct rpmsg_info *info = stream_timer->info;
	struct rpmsg_msg *msg;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		msg = &info->msg[TX_PERIOD_DONE + MSG_TYPE_A_NUM];
		msg->s_msg.header.cmd = TX_PERIOD_DONE;
	} else {
		msg = &info->msg[RX_PERIOD_DONE + MSG_TYPE_A_NUM];
		msg->s_msg.header.cmd = RX_PERIOD_DONE;
	}

	imx_rpmsg_insert_workqueue(substream, msg, info);
}
/* …（同文件无关代码省略）… */
static int imx_rpmsg_pcm_open(struct snd_soc_component *component,
			      struct snd_pcm_substream *substream)
{
	struct rpmsg_info *info = dev_get_drvdata(component->dev);
	struct snd_soc_pcm_runtime *rtd = snd_soc_substream_to_rtd(substream);
	struct snd_soc_dai *cpu_dai = snd_soc_rtd_to_cpu(rtd, 0);
	struct fsl_rpmsg *rpmsg = dev_get_drvdata(cpu_dai->dev);
	struct snd_pcm_hardware pcm_hardware;
	struct rpmsg_msg *msg;
	int ret = 0;
	int cmd;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		msg = &info->msg[TX_OPEN];
		msg->s_msg.header.cmd = TX_OPEN;

		/* reinitialize buffer counter*/
		cmd = TX_PERIOD_DONE + MSG_TYPE_A_NUM;
		info->msg[cmd].s_msg.param.buffer_tail = 0;
		info->msg[cmd].r_msg.param.buffer_tail = 0;
		info->msg[TX_POINTER].r_msg.param.buffer_offset = 0;

	} else {
		msg = &info->msg[RX_OPEN];
		msg->s_msg.header.cmd = RX_OPEN;

		/* reinitialize buffer counter*/
		cmd = RX_PERIOD_DONE + MSG_TYPE_A_NUM;
		info->msg[cmd].s_msg.param.buffer_tail = 0;
		info->msg[cmd].r_msg.param.buffer_tail = 0;
		info->msg[RX_POINTER].r_msg.param.buffer_offset = 0;
	}

	info->send_message(msg, info);

	pcm_hardware = imx_rpmsg_pcm_hardware;
	pcm_hardware.buffer_bytes_max = rpmsg->buffer_size[substream->stream];
	pcm_hardware.period_bytes_max = pcm_hardware.buffer_bytes_max / 2;

	snd_soc_set_runtime_hwparams(substream, &pcm_hardware);

	ret = snd_pcm_hw_constraint_integer(substream->runtime,
					    SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0)
		return ret;

	info->msg_drop_count[substream->stream] = 0;

	/* Create timer*/
	info->stream_timer[substream->stream].info = info;
	info->stream_timer[substream->stream].substream = substream;
	timer_setup(&info->stream_timer[substream->stream].timer,
		    imx_rpmsg_timer_callback, 0);

	/*
	 * ASoC only marks the component as opened when .open() returns 0.
	 * Since snd_pcm_hw_constraint_integer() may return 1, explicitly
	 * return 0 to ensure .close() is not skipped on rollback.
	 */
	return 0;
}
/* …（同文件无关代码省略）… */
static int imx_rpmsg_pcm_close(struct snd_soc_component *component,
			       struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = snd_soc_substream_to_rtd(substream);
	struct rpmsg_info *info = dev_get_drvdata(component->dev);
	struct rpmsg_msg *msg;

	/* Flush work in workqueue to make TX_CLOSE is the last message */
	flush_workqueue(info->rpmsg_wq);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		msg = &info->msg[TX_CLOSE];
		msg->s_msg.header.cmd = TX_CLOSE;
	} else {
		msg = &info->msg[RX_CLOSE];
		msg->s_msg.header.cmd = RX_CLOSE;
	}

	info->send_message(msg, info);

	timer_delete(&info->stream_timer[substream->stream].timer);

	rtd->dai_link->ignore_suspend = 0;

	if (info->msg_drop_count[substream->stream])
		dev_warn(rtd->dev, "Msg is dropped!, number is %d\n",
			 info->msg_drop_count[substream->stream]);

	return 0;
}
/* …（同文件无关代码省略）… */
static void imx_rpmsg_pcm_dma_complete(void *arg)
{
	struct snd_pcm_substream *substream = arg;

	snd_pcm_period_elapsed(substream);
}
/* …（同文件无关代码省略）… */
static int imx_rpmsg_prepare_and_submit(struct snd_soc_component *component,
					struct snd_pcm_substream *substream)
{
	struct rpmsg_info *info = dev_
