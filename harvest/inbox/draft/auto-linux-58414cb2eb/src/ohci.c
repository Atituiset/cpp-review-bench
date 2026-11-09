// AUTO-DRAFT from torvalds/linux PR #25fccf436e6e978954bb2774170d1094889c3fc7
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#define ohci_err(ohci, f, args...)	dev_err(ohci->card.device, f, ##args)
/* …（同文件无关代码省略）… */
struct descriptor {
	__le16 req_count;
	__le16 control;
	__le32 data_address;
	__le32 branch_address;
	__le16 res_count;
	__le16 transfer_status;
} __aligned(16);
/* …（同文件无关代码省略）… */
#define AR_BUFFER_SIZE	(32*1024)
#define AR_BUFFERS_MIN	DIV_ROUND_UP(AR_BUFFER_SIZE, PAGE_SIZE)
/* …（同文件无关代码省略）… */
#define AR_BUFFERS	MAX(2, AR_BUFFERS_MIN)
/* …（同文件无关代码省略）… */
struct ar_context {
	struct fw_ohci *ohci;
	struct page *pages[AR_BUFFERS];
	void *buffer;
	dma_addr_t dma_addrs[AR_BUFFERS];
	struct descriptor *descriptors;
	dma_addr_t descriptors_bus;
	void *pointer;
	unsigned int last_buffer_index;
	u32 regs;
	struct work_struct work;
};
/* …（同文件无关代码省略）… */
struct at_context {
	struct context context;
	struct work_struct work;
};
/* …（同文件无关代码省略）… */
struct at_local {
	struct list_head list;
	spinlock_t lock;
	struct work_struct work;
};
/* …（同文件无关代码省略）… */
struct iso_context {
	struct fw_iso_context base;
	struct context context;
	unsigned long flushing_completions;
	u8 sync;
	u8 tags;
	union {
		struct {
			u16 last_timestamp;
			size_t header_length;
			void *header;
		} sc;
		struct {
			u32 buffer_bus;
			u16 completed;
		} mc;
	};
};
/* …（同文件无关代码省略）… */
#define CONFIG_ROM_SIZE		(CSR_CONFIG_ROM_END - CSR_CONFIG_ROM)
/* …（同文件无关代码省略）… */
struct fw_ohci {
	struct fw_card card;

	__iomem char *registers;
	int node_id;
	int generation;
	int request_generation;	/* for timestamping incoming requests */
	unsigned quirks;
	unsigned int pri_req_max;
	u32 bus_time;
	bool bus_time_running;
	bool is_root;
	bool csr_state_setclear_abdicate;
	int n_ir;
	int n_it;
	/*
	 * Spinlock for accessing fw_ohci data.  Never call out of
	 * this driver with this lock held.
	 */
	spinlock_t lock;

	struct mutex phy_reg_mutex;

	void *misc_buffer;
	dma_addr_t misc_buffer_bus;

	struct ar_context ar_request_ctx;
	struct ar_context ar_response_ctx;
	struct at_context at_request_ctx;
	struct at_context at_response_ctx;

	struct at_local at_request_local;
	struct at_local at_response_local;

	u32 it_context_support;
	u32 it_context_mask;     /* unoccupied IT contexts */
	struct iso_context *it_context_list;
	u64 ir_context_channels; /* unoccupied channels */
	u32 ir_context_support;
	u32 ir_context_mask;     /* unoccupied IR contexts */
	struct iso_context *ir_context_list;
	u64 mc_channels; /* channels in use by the multichannel IR context */
	bool mc_allocated;

	__be32    *config_rom;
	dma_addr_t config_rom_bus;
	__be32    *next_config_rom;
	dma_addr_t next_config_rom_bus;
	__be32     next_header;

	__le32    *self_id;
	dma_addr_t self_id_bus;

	u32 self_id_buffer[512];
};
/* …（同文件无关代码省略）… */
#define QUIRK_CYCLE_TIMER		0x1
/* …（同文件无关代码省略）… */
#define has_reboot_by_cycle_timer_read_quirk(ohci) false
/* …（同文件无关代码省略）… */
static inline void reg_write(const struct fw_ohci *ohci, int offset, u32 data)
{
	writel(data, ohci->registers + offset);
}
/* …（同文件无关代码省略）… */
static inline u32 reg_read(const struct fw_ohci *ohci, int offset)
{
	return readl(ohci->registers + offset);
}
/* …（同文件无关代码省略）… */
static void handle_local_rom(struct fw_ohci *ohci, struct fw_packet *packet, u32 csr,
			     struct fw_packet *response)
{
	int tcode, rcode, length, i;
	void *payload = NULL;

	tcode = async_header_get_tcode(packet->header);
	if (tcode_is_block_packet(tcode))
		length = async_header_get_data_length(packet->header);
	else
		length = 4;

	i = csr - CSR_CONFIG_ROM;
	if (i + length > CONFIG_ROM_SIZE) {
		rcode = RCODE_ADDRESS_ERROR;
		length = 0;
	} else if (!tcode_is_read_request(tcode)) {
		rcode = RCODE_TYPE_ERROR;
		length = 0;
	} else {
		rcode = RCODE_COMPLETE;
		payload = (u8 *)ohci->config_rom + i;
	}

	fw_fill_response(response, packet->header, rcode, payload, length);
}
/* …（同文件无关代码省略）… */
static void handle_local_lock(struct fw_ohci *ohci, struct fw_packet *packet, u32 csr,
			      struct fw_packet *response)
{
	int tcode, length, ext_tcode, sel, try;
	__be32 *payload, lock_old;
	u32 lock_arg, lock_data;

	tcode = async_header_get_tcode(packet->header);
	length = async_header_get_data_length(packet->header);
	payload = packet->payload;
	ext_tcode = async_header_get_extended_tcode(packet->header);

	if (tcode == TCODE_LOCK_REQUEST &&
	    ext_tcode == EXTCODE_COMPARE_SWAP && length == 8) {
		lock_arg = be32_to_cpu(payload[0]);
		lock_data = be32_to_cpu(payload[1]);
	} else if (tcode == TCODE_READ_QUADLET_REQUEST) {
		lock_arg = 0;
		lock_data = 0;
	} else {
		fw_fill_response(response, packet->header, RCODE_TYPE_ERROR, NULL, 0);
		return;
	}

	sel = (csr - CSR_BUS_MANAGER_ID) / 4;
	reg_write(ohci, OHCI1394_CSRData, lock_data);
	reg_write(ohci, OHCI1394_CSRCompareData, lock_arg);
	reg_write(ohci, OHCI1394_CSRControl, sel);

	for (try = 0; try < 20; try++)
		if (reg_read(ohci, OHCI1394_CSRControl) & 0x80000000) {
			lock_old = cpu_to_be32(reg_read(ohci,
							OHCI1394_CSRData));
			fw_fill_response(response, packet->header, RCODE_COMPLETE, &lock_old,
					 sizeof(lock_old));
			return;
		}

	ohci_err(ohci, "swap not done (CSR lock timeout)\n");
	fw_fill_response(response, packet->header, RCODE_BUSY, NULL, 0);
}
/* …（同文件无关代码省略）… */
static bool in_config_rom_csr_registers(u64 offset)
{
	return in_range(offset, CSR_CONFIG_ROM, CONFIG_ROM_SIZE);
}
/* …（同文件无关代码省略）… */
static bool in_bus_management_csr_registers(u64 offset)
{
	// 0x22c = CSR_MAINT_CONTROL following to CSR_CHANNELS_AVAILABLE_LO.
	return in_range(offset, CSR_BUS_MANAGER_ID, 0x22c - CSR_BUS_MANAGER_ID);
}
/* …（同文件无关代码省略）… */
static void handle_at_request_local_packet(struct fw_ohci *ohci, struct fw_packet *packet)
{
	// Emulate split transaction.
	packet->ack = ACK_PENDING;
	packet->callback(packet, &ohci->card, packet->ack);

	u64 csr_offset = async_header_get_offset(packet->header) - CSR_REGISTER_BASE;

	if (in_config_rom_csr_registers(csr_offset) || in_bus_management_csr_registers(csr_offset)) {
		struct fw_packet response;

		memset(&response, 0, sizeof(response));

		if (in_bus_management_csr_registers(csr_offset))
			handle_local_lock(ohci, packet, csr_offset, &response);
		else
			handle_local_rom(ohci, packet, csr_offset, &response);

		// Timestamping on behalf of the hardware.
		response.timestamp = cycle_time_to_ohci_tstamp(get_cycle_time(ohci));

		// Finish the transaction immediately.
		fw_core_handle_response(&ohci->card, &response);
	} else {
		fw_core_handle_request(&ohci->card, packet);
	}
}
/* …（同文件无关代码省略）… */
static void handle_at_local_packets(struct at_local *local, struct fw_ohci *ohci,
			void (*handle_at_local_packet)(struct fw_ohci *, struct fw_packet *))
{
	struct fw_packet *packet;

	spin_lock(&local->lock);

	while ((packet = list_first_entry_or_null(&local->list, typeof(*packet), link_for_local))) {
		list_del_init(&packet->link_for_local);
		spin_unlock(&local->lock);

		if (unlikely(packet->ack != 0)) {
			// This case is active when the call of at_context_queue_packet() returns
			// error in at_context_transmit().
			packet->callback(packet, &ohci->card, packet->ack);
		} else {
			handle_at_local_packet(ohci, packet);
		}

		spin_lock(&local->lock);
	}

	spin_unlock(&local->lock);
}

static void at_request_local_work(struct work_struct *work)
{
	struct at_local *local = from_work(local, work, work);
	struct fw_ohci *ohci = container_of(local, struct fw_ohci, at_request_local);

	handle_at_local_packets(local, ohci, handle_at_request_local_packet);
}
/* …（同文件无关代码省略）… */
static u32 cycle_timer_ticks(u32 cycle_timer)
{
	u32 ticks;

	ticks = cycle_timer & 0xfff;
	ticks += 3072 * ((cycle_timer >> 12) & 0x1fff);
	ticks += (3072 * 8000) * (cycle_timer >> 25);

	return ticks;
}
/* …（同文件无关代码省略）… */
static u32 get_cycle_time(struct fw_ohci *ohci)
{
	u32 c0, c1, c2;
	u32 t0, t1, t2;
	s32 diff01, diff12;
	int i;

	if (has_reboot_by_cycle_timer_read_quirk(ohci))
		return 0;

	c2 = reg_read(ohci, OHCI1394_IsochronousCycleTimer);

	if (ohci->quirks & QUIRK_CYCLE_TIMER) {
		i = 0;
		c1 = c2;
		c2 = reg_read(ohci, OHCI1394_IsochronousCycleTimer);
		do {
			c0 = c1;
			c1 = c2;
			c2 = reg_read(ohci, OHCI1394_IsochronousCycleTimer);
			t0 = cycle_timer_ticks(c0);
			t1 = cycle_timer_ticks(c1);
			t2 = cycle_timer_ticks(c2);
			diff01 = t1 - t0;
			diff12 = t2 - t1;
		} while ((diff01 <= 0 || diff12 <= 0 ||
			  diff01 / diff12 >= 2 || diff12 / diff01 >= 2)
			 && i++ < 20);
	}

	return c2;
}
