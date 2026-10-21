// AUTO-DRAFT from torvalds/linux PR #331eadd85f14bac85663c14c99228f6dedf64d87
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
struct avc_entry {
	u32			ssid;
	u32			tsid;
	u16			tclass;
	struct av_decision	avd;
	struct avc_xperms_node	*xp_node;
};
/* …（同文件无关代码省略）… */
struct avc_node {
	struct avc_entry	ae;
	struct hlist_node	list; /* anchored in avc_cache->slots[i] */
	struct rcu_head		rhead;
};
/* …（同文件无关代码省略）… */
struct avc_xperms_decision_node {
	struct extended_perms_decision xpd;
	struct list_head xpd_list; /* list of extended_perms_decision */
};
/* …（同文件无关代码省略）… */
struct avc_xperms_node {
	struct extended_perms xp;
	struct list_head xpd_head; /* list head of extended_perms_decision */
};
/* …（同文件无关代码省略）… */
}

static inline unsigned int
avc_xperms_has_perm(struct extended_perms_decision *xpd,
					u8 perm, u8 which)
{
	unsigned int rc = 0;

/* …（同文件无关代码省略）… */
}

static void avc_copy_xperms_decision(struct extended_perms_decision *dest,
					struct extended_perms_decision *src)
{
	dest->base_perm = src->base_perm;
	dest->driver = src->driver;
/* …（同文件无关代码省略）… */
 */
static inline void avc_quick_copy_xperms_decision(u8 perm,
			struct extended_perms_decision *dest,
			struct extended_perms_decision *src)
{
	/*
	 * compute index of the u32 of the 256 bits (8 u32s) that contain this
/* …（同文件无关代码省略）… */
}

static int avc_add_xperms_decision(struct avc_node *node,
			struct extended_perms_decision *src)
{
	struct avc_xperms_decision_node *dest_xpd;

/* …（同文件无关代码省略）… */
}

static int avc_xperms_populate(struct avc_node *node,
				struct avc_xperms_node *src)
{
	struct avc_xperms_node *dest;
	struct avc_xperms_decision_node *dest_xpd;
/* …（同文件无关代码省略）… */
}

static inline u32 avc_xperms_audit_required(u32 requested,
					struct av_decision *avd,
					struct extended_perms_decision *xpd,
					u8 perm,
					int result,
					u32 *deniedp)
{
	u32 denied, audited;

	denied = requested & ~avd->allowed;
	if (unlikely(denied)) {
		audited = denied & avd->auditdeny;
/* …（同文件无关代码省略）… */
}

static inline int avc_xperms_audit(u32 ssid, u32 tsid, u16 tclass,
				   u32 requested, struct av_decision *avd,
				   struct extended_perms_decision *xpd,
				   u8 perm, int result,
				   struct common_audit_data *ad)
{
/* …（同文件无关代码省略）… */
	return node;
}

static void avc_node_populate(struct avc_node *node, u32 ssid, u32 tsid, u16 tclass, struct av_decision *avd)
{
	node->ae.ssid = ssid;
	node->ae.tsid = tsid;
/* …（同文件无关代码省略）… */
 * the access vectors into a cache entry.
 */
static void avc_insert(u32 ssid, u32 tsid, u16 tclass,
		       struct av_decision *avd, struct avc_xperms_node *xp_node)
{
	struct avc_node *pos, *node = NULL;
	u32 hvalue;
/* …（同文件无关代码省略）… */
 */
static int avc_update_node(u32 event, u32 perms, u8 driver, u8 base_perm,
			   u8 xperm, u32 ssid, u32 tsid, u16 tclass, u32 seqno,
			   struct extended_perms_decision *xpd, u32 flags)
{
	u32 hvalue;
	int rc = 0;
