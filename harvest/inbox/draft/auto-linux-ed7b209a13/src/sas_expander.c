// AUTO-DRAFT from torvalds/linux PR #f909e534f5e34feb2ea2701d54df63e21aa506fc
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define SMP_TIMEOUT 10
/* …（同文件无关代码省略）… */
static int smp_execute_task_sg(struct domain_device *dev,
		struct scatterlist *req, struct scatterlist *resp)
{
	int res, retry;
	struct sas_task *task = NULL;
	struct sas_internal *i =
		to_sas_internal(dev->port->ha->shost->transportt);
	struct sas_ha_struct *ha = dev->port->ha;

	pm_runtime_get_sync(ha->dev);
	mutex_lock(&dev->ex_dev.cmd_mutex);
	for (retry = 0; retry < 3; retry++) {
		if (test_bit(SAS_DEV_GONE, &dev->state)) {
			res = -ECOMM;
			break;
		}

		task = sas_alloc_slow_task(GFP_KERNEL);
		if (!task) {
			res = -ENOMEM;
			break;
		}
		task->dev = dev;
		task->task_proto = dev->tproto;
		task->smp_task.smp_req = *req;
		task->smp_task.smp_resp = *resp;

		task->task_done = sas_task_internal_done;

		task->slow_task->timer.function = sas_task_internal_timedout;
		task->slow_task->timer.expires = jiffies + SMP_TIMEOUT*HZ;
		add_timer(&task->slow_task->timer);

		res = i->dft->lldd_execute_task(task, GFP_KERNEL);

		if (res) {
			timer_delete_sync(&task->slow_task->timer);
			pr_notice("executing SMP task failed:%d\n", res);
			break;
		}

		wait_for_completion(&task->slow_task->completion);
		res = -ECOMM;
		if ((task->task_state_flags & SAS_TASK_STATE_ABORTED)) {
			pr_notice("smp task timed out or aborted\n");
			i->dft->lldd_abort_task(task);
			if (!(task->task_state_flags & SAS_TASK_STATE_DONE)) {
				pr_notice("SMP task aborted and not done\n");
				break;
			}
		}
		if (task->task_status.resp == SAS_TASK_COMPLETE &&
		    task->task_status.stat == SAS_SAM_STAT_GOOD) {
			res = 0;
			break;
		}
		if (task->task_status.resp == SAS_TASK_COMPLETE &&
		    task->task_status.stat == SAS_DATA_UNDERRUN) {
			/* no error, but return the number of bytes of
			 * underrun */
			res = task->task_status.residual;
			break;
		}
		if (task->task_status.resp == SAS_TASK_COMPLETE &&
		    task->task_status.stat == SAS_DATA_OVERRUN) {
			res = -EMSGSIZE;
			break;
		}
		if (task->task_status.resp == SAS_TASK_UNDELIVERED &&
		    task->task_status.stat == SAS_DEVICE_UNKNOWN)
			break;
		else {
			pr_notice("%s: task to dev %016llx response: 0x%x status 0x%x\n",
				  __func__,
				  SAS_ADDR(dev->sas_addr),
				  task->task_status.resp,
				  task->task_status.stat);
			sas_free_task(task);
			task = NULL;
		}
	}
	mutex_unlock(&dev->ex_dev.cmd_mutex);
	pm_runtime_put_sync(ha->dev);

	BUG_ON(retry == 3 && task != NULL);
	sas_free_task(task);
	return res;
}
/* …（同文件无关代码省略）… */
		goto out;
	}

	ret = smp_execute_task_sg(dev, job->request_payload.sg_list,
			job->reply_payload.sg_list);
	if (ret >= 0) {
		/* bsg_job_done() requires the length received  */
		rcvlen = job->reply_payload.payload_len - ret;
		ret = 0;
	}

out:
	bsg_job_done(job, ret, rcvlen);
