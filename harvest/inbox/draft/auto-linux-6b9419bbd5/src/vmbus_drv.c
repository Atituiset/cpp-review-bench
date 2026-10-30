// AUTO-DRAFT from torvalds/linux PR #9c7e3d59107e2daf204bf6bf87b6c448ca89defe
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
#include <linux/sysfb.h>
#include <linux/efi.h>
#include <linux/kernel.h>
#include <linux/syscore_ops.h>
#include <linux/dma-map-ops.h>
#include <linux/pci.h>
/* …（同文件无关代码省略）… */
static void vmbus_chan_sched(void *event_page_addr)
{
	unsigned long *recv_int_page;
	u32 maxbits, relid;
	union hv_synic_event_flags *event;

	if (!event_page_addr)
		return;
	event = (union hv_synic_event_flags *)event_page_addr + VMBUS_MESSAGE_SINT;

	maxbits = READ_ONCE(vmbus_connection.relid_hiwater) + 1;
	recv_int_page = event->flags;

	if (unlikely(!recv_int_page))
		return;

	for_each_set_bit(relid, recv_int_page, maxbits) {
		void (*callback_fn)(void *context);
		struct vmbus_channel *channel;

		if (!sync_test_and_clear_bit(relid, recv_int_page))
			continue;

		/* Special case - vmbus channel protocol msg */
		if (relid == 0)
			continue;

		/*
		 * Pairs with the kfree_rcu() in vmbus_chan_release().
		 * Guarantees that the channel data structure doesn't
		 * get freed while the channel pointer below is being
		 * dereferenced.
		 */
		rcu_read_lock();

		/* Find channel based on relid */
		channel = relid2channel(relid);
		if (channel == NULL)
			goto sched_unlock_rcu;

		if (channel->rescind)
			goto sched_unlock_rcu;

		/*
		 * Make sure that the ring buffer data structure doesn't get
		 * freed while we dereference the ring buffer pointer.  Test
		 * for the channel's onchannel_callback being NULL within a
		 * sched_lock critical section.  See also the inline comments
		 * in vmbus_reset_channel_cb().
		 */
		spin_lock(&channel->sched_lock);

		callback_fn = channel->onchannel_callback;
		if (unlikely(callback_fn == NULL))
			goto sched_unlock;

		trace_vmbus_chan_sched(channel);

		++channel->interrupts;

		switch (channel->callback_mode) {
		case HV_CALL_ISR:
			(*callback_fn)(channel->channel_callback_context);
			break;

		case HV_CALL_BATCHED:
			hv_begin_read(&channel->inbound);
			fallthrough;
		case HV_CALL_DIRECT:
			tasklet_schedule(&channel->callback_event);
		}

sched_unlock:
		spin_unlock(&channel->sched_lock);
sched_unlock_rcu:
		rcu_read_unlock();
	}
}
/* …（同文件无关代码省略）… */
static void vmbus_message_sched(struct hv_per_cpu_context *hv_cpu, void *message_page_addr)
{
	struct hv_message *msg;

	if (!message_page_addr)
		return;
	msg = (struct hv_message *)message_page_addr + VMBUS_MESSAGE_SINT;

	/* Check if there are actual msgs to be processed */
	if (msg->header.message_type != HVMSG_NONE)
		tasklet_schedule(&hv_cpu->msg_dpc);
}
/* …（同文件无关代码省略）… */
static void __vmbus_isr(void)
{
	struct hv_per_cpu_context *hv_cpu
		= this_cpu_ptr(hv_context.cpu_context);

	vmbus_chan_sched(hv_cpu->hyp_synic_event_page);
	vmbus_chan_sched(hv_cpu->para_synic_event_page);

	vmbus_message_sched(hv_cpu, hv_cpu->hyp_synic_message_page);
	vmbus_message_sched(hv_cpu, hv_cpu->para_synic_message_page);
}
/* …（同文件无关代码省略）… */
static void vmbus_irqd_wake(void)
{
	struct task_struct *tsk = __this_cpu_read(vmbus_irqd);

	__this_cpu_write(vmbus_irq_pending, true);
	wake_up_process(tsk);
}
/* …（同文件无关代码省略）… */
void vmbus_isr(void)
{
	if (IS_ENABLED(CONFIG_PREEMPT_RT)) {
		vmbus_irqd_wake();
	} else {
		static DEFINE_WAIT_OVERRIDE_MAP(vmbus_map, LD_WAIT_CONFIG);

		/*
		 * vmbus_isr is never force-threaded and always invoked at hard
		 * IRQ level. __vmbus_isr() below can acquire a spinlock_t
		 * which becomes a sleeping lock and must not be acquired in
		 * this context. Therefore on PREEMPT_RT this will be threaded
		 * via vmbus_irqd_wake(). On non-PREEMPT the annotation lets
		 * lockdep know that acquiring a spinlock_t is not an issue.
		 */
		lock_map_acquire_try(&vmbus_map);
		__vmbus_isr();
		lock_map_release(&vmbus_map);
	}
}
/* …（同文件无关代码省略）… */
static irqreturn_t vmbus_percpu_isr(int irq, void *dev_id)
{
	vmbus_isr();
	return IRQ_HANDLED;
}
/* …（同文件无关代码省略）… */
static void vmbus_percpu_work(struct work_struct *work)
{
	unsigned int cpu = smp_processor_id();

	hv_synic_init(cpu);
}
/* …（同文件无关代码省略）… */
static int vmbus_alloc_synic_and_connect(void)
{
	int ret, cpu;
	struct work_struct __percpu *works;

	ret = hv_synic_alloc();
	if (ret < 0)
		goto err_alloc;

	works = alloc_percpu(struct work_struct);
	if (!works) {
		ret = -ENOMEM;
		goto err_alloc;
	}

	/*
	 * Initialize the per-cpu interrupt state and stimer state.
	 * Then connect to the host.
	 */
	cpus_read_lock();
	for_each_online_cpu(cpu) {
		struct work_struct *work = per_cpu_ptr(works, cpu);

		INIT_WORK(work, vmbus_percpu_work);
		schedule_work_on(cpu, work);
	}

	for_each_online_cpu(cpu)
		flush_work(per_cpu_ptr(works, cpu));

	/* Register the callbacks for possible CPU online/offline'ing */
	ret = cpuhp_setup_state_nocalls_cpuslocked(CPUHP_AP_ONLINE_DYN, "hyperv/vmbus:online",
						   hv_synic_init, hv_synic_cleanup);
	cpus_read_unlock();
	free_percpu(works);
	if (ret < 0)
		goto err_alloc;
	hyperv_cpuhp_online = ret;

	ret = vmbus_connect();
	if (ret)
		goto err_connect;
	return 0;

err_connect:
	cpuhp_remove_state(hyperv_cpuhp_online);
	return -ENODEV;
err_alloc:
	hv_synic_free();
	return -ENOMEM;
}
/* …（同文件无关代码省略）… */
static int vmbus_bus_init(void)
{
	int ret;

	ret = hv_init();
	if (ret != 0) {
		pr_err("Unable to initialize the hypervisor - 0x%x\n", ret);
		return ret;
	}

	ret = bus_register(&hv_bus);
	if (ret)
		return ret;

	/*
	 * VMbus interrupts are best modeled as per-cpu interrupts. If
	 * on an architecture with support for per-cpu IRQs (e.g. ARM64),
	 * allocate a per-cpu IRQ using standard Linux kernel functionality.
	 * If not on such an architecture (e.g., x86/x64), then rely on
	 * code in the arch-specific portion of the code tree to connect
	 * the VMbus interrupt handler.
	 */

	if (IS_ENABLED(CONFIG_PREEMPT_RT)) {
		ret = smpboot_register_percpu_thread(&vmbus_irq_threads);
		if (ret)
			goto err_kthread;
	}

	if (vmbus_irq == -1) {
		hv_setup_vmbus_handler(vmbus_isr);
	} else {
		ret = request_percpu_irq(vmbus_irq, vmbus_percpu_isr,
				"Hyper-V VMbus", &vmbus_evt);
		if (ret) {
			pr_err("Can't request Hyper-V VMbus IRQ %d, Err %d",
					vmbus_irq, ret);
			goto err_setup;
		}
	}

	/*
	 * Cache the value as getting it involves a VM exit on x86(_64), and
	 * doing that on each VP while initializing SynIC's wastes time.
	 */
	is_confidential = ms_hyperv.confidential_vmbus_available;
	if (is_confidential)
		pr_info("Establishing connection to the confidential VMBus\n");
	hv_para_set_sint_proxy(!is_confidential);
	ret = vmbus_alloc_synic_and_connect();
	if (ret)
		goto err_connect;

	/*
	 * Always register the vmbus unload panic notifier because we
	 * need to shut the VMbus channel connection on panic.
	 */
	atomic_notifier_chain_register(&panic_notifier_list,
			       &hyperv_panic_vmbus_unload_block);

	vmbus_request_offers();

	return 0;

err_connect:
	if (vmbus_irq == -1)
		hv_remove_vmbus_handler();
	else
		free_percpu_irq(vmbus_irq, &vmbus_evt);
err_setup:
	if (IS_ENABLED(CONFIG_PREEMPT_RT))
		smpboot_unregister_percpu_thread(&vmbus_irq_threads);
err_kthread:
	bus_unregister(&hv_bus);
	return ret;
}
/* …（同文件无关代码省略）… */
#ifdef CONFIG_ACPI
static int vmbus_acpi_add(struct platform_device *pdev)
{
	acpi_status result;
	int ret_val = -ENODEV;
	struct acpi_device *ancestor;
	struct acpi_device *device = ACPI_COMPANION(&pdev->dev);

	vmbus_root_device = &device->dev;

/* …（同文件无关代码省略）… */
static void hv_kexec_handler(void)
{
	vmbus_initiate_unload(false);
	/* Make sure conn_state is set as hv_synic_cleanup checks for it */
	mb();
	cpuhp_remove_state(hyperv_cpuhp_online);
};

static void hv_crash_handler(struct pt_regs *regs)
{
	int cpu;
/* …（同文件无关代码省略）… */
static int hv_synic_suspend(void *data)
{
	/*
	 * When we reach here, all the non-boot CPUs have been offlined,
	 * and their stimers have been cleaned up by the cpuhp teardown
	 * callback hv_stimer_cleanup().
	 *
	 * hv_synic_suspend() only runs on CPU0 with interrupts disabled.
	 * The stimer on CPU0 is not explicitly cleaned up here because:
	 * 1) it's harmless as interrupts remain disabled between
	 * syscore_suspend() and syscore_resume(), so the stimer cannot
	 * fire during this window: see create_image() and
	 * resume_target_kernel()
	 * 2) the stimer on CPU0 is automatically disabled later by
	 * syscore_suspend() -> timekeeping_suspend() -> tick_suspend() -> ...
	 * -> clockevents_shutdown() -> ... -> hv_ce_shutdown()
	 */

	hv_hyp_synic_disable_regs(0);

	return 0;
}
/* …（同文件无关代码省略）… */
static void hv_synic_resume(void *data)
{
	hv_hyp_synic_enable_regs(0);

	/*
	 * Note: we don't need to call hv_stimer_init(0), because the timer
	 * on CPU0 is not unbound in hv_synic_suspend(), and the timer is
	 * automatically re-enabled in timekeeping_resume().
	 */
}

/* The callbacks run only on CPU0, with irqs_disabled. */
static const struct syscore_ops hv_synic_syscore_ops = {
	.suspend = hv_synic_suspend,
	.resume = hv_synic_resume,
};

static struct syscore hv_synic_syscore = {
	.ops = &hv_synic_syscore_ops,
};

static int __init hv_acpi_init(void)
{
	int ret;

	if (!hv_is_hyperv_initialized())
		return -ENODEV;

	if (hv_root_partition() && !hv_nested)
		/*
		 * A non-nested root partition does not need VMBus client
		 * functionality. However, the mshv_root module may have
		 * a dependency on the VMBus module as described in
		 * commit 840b740a35bf. Return success so the module
		 * loads even though no VMBus initialization is done.
		 */
		return 0;

	/*
	 * Get ACPI resources first.
	 */
	ret = platform_driver_register(&vmbus_platform_driver);
	if (ret)
		return ret;

	if (!vmbus_root_device) {
		ret = -ENODEV;
		goto cleanup;
	}

	/*
	 * If we're on an architecture with a hardcoded hypervisor
	 * vector (i.e. x86/x64), override the VMbus interrupt found
	 * in the ACPI tables. Ensure vmbus_irq is not set since the
	 * normal Linux IRQ mechanism is not used in this case.
	 */
#ifdef HYPERVISOR_CALLBACK_VECTOR
	vmbus_interrupt = HYPERVISOR_CALLBACK_VECTOR;
	vmbus_irq = -1;
#endif

	hv_debug_init();

	ret = vmbus_bus_init();
	if (ret)
		goto cleanup;

	hv_setup_kexec_handler(hv_kexec_handler);
	hv_setup_crash_handler(hv_crash_handler);

	register_syscore(&hv_synic_syscore);

	return 0;

cleanup:
	platform_driver_unregister(&vmbus_platform_driver);
	vmbus_root_device = NULL;
	return ret;
}
/* …（同文件无关代码省略）… */
		 */
		return;

	unregister_syscore(&hv_synic_syscore);

	hv_remove_kexec_handler();
	hv_remove_crash_handler();
	hv_stimer_global_cleanup();
	vmbus_disconnect();
