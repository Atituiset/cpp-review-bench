// AUTO-DRAFT from torvalds/linux PR #2270aa5ed5f7981082e4fae4103b8930e0ba106f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stdint.h>
  // <<< BUG ANCHOR
#include <linux/libfdt.h>
#include <linux/kexec.h>

#include <asm/processor.h>
#include <asm/reboot.h>
#include <asm/smp-ops.h>
/* …（同文件无关代码省略）… */
	write_c0_derraddr1(0);
}

/**
 * prom_init - Early entry point for arch setup
 */
/* …（同文件无关代码省略）… */
	}

#ifdef CONFIG_CAVIUM_OCTEON_LOCK_L2
	if (cvmx_read_csr(CVMX_L2D_FUS3) & (3ull << 34)) {
		pr_info("Skipping L2 locking due to reduced L2 cache size\n");
	} else {
		uint32_t __maybe_unused ebase = read_c0_ebase() & 0x3ffff000;
/* …（同文件无关代码省略）… */

void __init prom_free_prom_memory(void)
{
	if (OCTEON_IS_MODEL(OCTEON_CN6XXX)) {
		/* Check for presence of Core-14449 fix.  */
		u32 insn;
		u32 *foo;
/* …（同文件无关代码省略）… */

void __init device_tree_init(void)
{
	const void *fdt;
	bool do_prune;
	bool fill_mac;

/* …（同文件无关代码省略）… */
		do_prune = false;
		fill_mac = false;
		pr_info("Using passed Device Tree.\n");
	} else if (OCTEON_IS_MODEL(OCTEON_CN68XX)) {
		fdt = &__dtb_octeon_68xx_begin;
		do_prune = true;
		fill_mac = true;
	} else {
		fdt = &__dtb_octeon_3xxx_begin;
		do_prune = true;
		fill_mac = true;
	}
