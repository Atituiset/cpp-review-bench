// AUTO-DRAFT from torvalds/linux PR #331eadd85f14bac85663c14c99228f6dedf64d87
  // <<< BUG ANCHOR
void __init avc_init(void);

static inline u32 avc_audit_required(u32 requested, struct av_decision *avd,
				     int result, u32 auditdeny, u32 *deniedp)
{
	u32 denied, audited;

/* …（同文件无关代码省略）… */
 * before calling the auditing code.
 */
static inline int avc_audit(u32 ssid, u32 tsid, u16 tclass, u32 requested,
			    struct av_decision *avd, int result,
			    struct common_audit_data *a)
{
	u32 audited, denied;
