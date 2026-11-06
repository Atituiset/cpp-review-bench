// AUTO-DRAFT from torvalds/linux PR #3e516eae690b3f24c19168cc0fe605e1c610eb21
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
	int temp;
	bool temp_valid;
	int tjmax;
	unsigned long last_updated;
	unsigned int cpu;
	int index;
	u32 cpu_core_id;
/* …（同文件无关代码省略）… */
struct tjmax_model {
	u32 vfm;
	u8 stepping_mask;
	int tjmax;
};
/* …（同文件无关代码省略）… */
#define ANY 0xff
/* …（同文件无关代码省略）… */
static int adjust_tjmax(struct cpuinfo_x86 *c, u32 id, struct device *dev)
{
	/* The 100C is default for both mobile and non mobile CPUs */

	int tjmax = 100000;
	int tjmax_ee = 85000;
	int usemsr_ee = 1;
	int err;
	u64 val;
	int i;
	u16 devfn = PCI_DEVFN(0, 0);
	struct pci_dev *host_bridge = pci_get_domain_bus_and_slot(0, 0, devfn);

	/*
	 * Explicit tjmax table entries override heuristics.
	 * First try PCI host bridge IDs, followed by model ID strings
	 * and model/stepping information.
	 */
	if (host_bridge && host_bridge->vendor == PCI_VENDOR_ID_INTEL) {
		for (i = 0; i < ARRAY_SIZE(tjmax_pci_table); i++) {
			if (host_bridge->device == tjmax_pci_table[i].device) {
				pci_dev_put(host_bridge);
				return tjmax_pci_table[i].tjmax;
			}
		}
	}
	pci_dev_put(host_bridge);

	/*
	 * This is literally looking for "CPU  XXX" in the model string.
	 * Not checking it against the model as well. Just purely a
	 * string search.
	 */
	for (i = 0; i < ARRAY_SIZE(tjmax_table); i++) {
		if (strstr(c->x86_model_id, tjmax_table[i].id))
			return tjmax_table[i].tjmax;
	}

	for (i = 0; i < ARRAY_SIZE(tjmax_model_table); i++) {
		const struct tjmax_model *tm = &tjmax_model_table[i];

		if (c->x86_vfm == tm->vfm &&
		    (tm->stepping_mask == ANY ||
		     tm->stepping_mask == c->x86_stepping))
			return tm->tjmax;
	}

	/* Early chips have no MSR for TjMax */

	if (c->x86_vfm == INTEL_CORE2_MEROM && c->x86_stepping < 4)
		usemsr_ee = 0;

	if (c->x86_vfm > INTEL_CORE_YONAH && usemsr_ee) {
		u8 platform_id;

		/*
		 * Now we can detect the mobile CPU using Intel provided table
		 * http://softwarecommunity.intel.com/Wiki/Mobility/720.htm
		 * For Core2 cores, check MSR 0x17, bit 28 1 = Mobile CPU
		 */
		err = rdmsrq_safe_on_cpu(id, 0x17, &val);
		if (err) {
			dev_warn(dev,
				 "Unable to access MSR 0x17, assuming desktop CPU\n");
			usemsr_ee = 0;
		} else if (c->x86_vfm < INTEL_CORE2_PENRYN &&
			   !(val & 0x10000000)) {
			/*
			 * Trust bit 28 up to Penryn, I could not find any
			 * documentation on that; if you happen to know
			 * someone at Intel please ask
			 */
			usemsr_ee = 0;
		} else {
			/* Platform ID bits 52:50 */
			platform_id = (val >> 50) & 0x7;

			/*
			 * Mobile Penryn CPU seems to be platform ID 7 or 5
			 * (guesswork)
			 */
			if (c->x86_vfm == INTEL_CORE2_PENRYN &&
			    (platform_id == 5 || platform_id == 7)) {
				/*
				 * If MSR EE bit is set, set it to 90 degrees C,
				 * otherwise 105 degrees C
				 */
				tjmax_ee = 90000;
				tjmax = 105000;
			}
		}
	}

	if (usemsr_ee) {
		err = rdmsrq_safe_on_cpu(id, 0xee, &val);
		if (err) {
			dev_warn(dev,
				 "Unable to access MSR 0xEE, for Tjmax, left at default\n");
		} else if (val & 0x40000000) {
			tjmax = tjmax_ee;
		}
	} else if (tjmax == 100000) {
		/*
		 * If we don't use msr EE it means we are desktop CPU
		 * (with exeception of Atom)
		 */
		dev_warn(dev, "Using relative temperature scale!\n");
	}

	return tjmax;
}
/* …（同文件无关代码省略）… */
static int get_tjmax(struct temp_data *tdata, struct device *dev)
{
	struct cpuinfo_x86 *c = &cpu_data(tdata->cpu);
	int err;
	u64 msrval;
	u32 val;

	/* use static tjmax once it is set */
	if (tdata->tjmax)
		return tdata->tjmax;

	/*
	 * A new feature of current Intel(R) processors, the
	 * IA32_TEMPERATURE_TARGET contains the TjMax value
	 */
	err = rdmsrq_safe_on_cpu(tdata->cpu, MSR_IA32_TEMPERATURE_TARGET, &msrval);
	if (err) {
		dev_warn_once(dev, "Unable to read TjMax from CPU %u\n", tdata->cpu);
	} else {
		val = (msrval >> 16) & 0xff;
		if (val)
			return val * 1000;
	}

	if (force_tjmax) {
		dev_notice(dev, "TjMax forced to %d degrees C by user\n",
			   force_tjmax);
		tdata->tjmax = force_tjmax * 1000;
	} else {
		/*
		 * An assumption is made for early CPUs and unreadable MSR.
		 * NOTE: the calculated value may not be correct.
		 */
		tdata->tjmax = adjust_tjmax(c, tdata->cpu, dev);
	}
	return tdata->tjmax;
}
/* …（同文件无关代码省略）… */
	mutex_lock(&tdata->update_lock);

	/* Check whether the time interval has elapsed */
	if (time_after(jiffies, tdata->last_updated + HZ)) {
		tjmax = get_tjmax(tdata, dev);
		rdmsrq_on_cpu(tdata->cpu, tdata->status_reg, &val.q);
		/*
/* …（同文件无关代码省略）… */
		 */
		tdata->temp_valid = (val.l >> 31) & 1;
		tdata->temp = tjmax - ((val.l >> 16) & 0xff) * 1000;
		tdata->last_updated = jiffies;
	}

	valid = tdata->temp_valid;
/* …（同文件无关代码省略）… */
	struct temp_data *tdata;

	if (!pdata->core_data) {
		pdata->nr_cores = topology_num_cores_per_package();
		pdata->core_data = kzalloc_objs(struct temp_data *,
						pdata->nr_cores);
		if (!pdata->core_data)
			return NULL;
	}

	tdata = kzalloc_obj(struct temp_data);
/* …（同文件无关代码省略）… */
	tdata->cpu = cpu;
	tdata->cpu_core_id = topology_core_id(cpu);
	tdata->attr_size = MAX_CORE_ATTRS;
	/*
	 * A zero timestamp does not look stale on 32-bit, where jiffies
	 * starts just short of wrapping. Backdate it instead.
	 */
	tdata->last_updated = jiffies - HZ - 1;
	mutex_init(&tdata->update_lock);
	return tdata;
}
/* …（同文件无关代码省略）… */
	if (!housekeeping_cpu(cpu, HK_TYPE_MISC))
		return 0;

	pdata = platform_get_drvdata(pdev);
	if (!pdata->hwmon_dev) {
		struct device *hwmon;
/* …（同文件无关代码省略）… */
		return 0;

	/* If the physical CPU device does not exist, just return */
	pd = platform_get_drvdata(pdev);
	if (!pd->hwmon_dev)
		return 0;
