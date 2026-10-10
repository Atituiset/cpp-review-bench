// AUTO-DRAFT from torvalds/linux PR #af32da41b0327b9c6a37856ba82b6760d6c8d10e
enum mlxsw_sp_acl_mangle_field {
	MLXSW_SP_ACL_MANGLE_FIELD_IP_DSFIELD,
	MLXSW_SP_ACL_MANGLE_FIELD_IP_DSCP,
	MLXSW_SP_ACL_MANGLE_FIELD_IP_ECN,
	MLXSW_SP_ACL_MANGLE_FIELD_IP_SPORT,
	MLXSW_SP_ACL_MANGLE_FIELD_IP_DPORT,
	MLXSW_SP_ACL_MANGLE_FIELD_IP4_SIP,
	MLXSW_SP_ACL_MANGLE_FIELD_IP4_DIP,
	MLXSW_SP_ACL_MANGLE_FIELD_IP6_SIP_1,
	MLXSW_SP_ACL_MANGLE_FIELD_IP6_SIP_2,
	MLXSW_SP_ACL_MANGLE_FIELD_IP6_SIP_3,
	MLXSW_SP_ACL_MANGLE_FIELD_IP6_SIP_4,
	MLXSW_SP_ACL_MANGLE_FIELD_IP6_DIP_1,
	MLXSW_SP_ACL_MANGLE_FIELD_IP6_DIP_2,
	MLXSW_SP_ACL_MANGLE_FIELD_IP6_DIP_3,
	MLXSW_SP_ACL_MANGLE_FIELD_IP6_DIP_4,
};
/* …（同文件无关代码省略）… */
struct mlxsw_sp_acl_rule_info {
	unsigned int priority;
	struct mlxsw_afk_element_values values;
	struct mlxsw_afa_block *act_block;
	u8 action_created:1,
	   ingress_bind_blocker:1,
	   egress_bind_blocker:1,
	   counter_valid:1,
	   policer_index_valid:1,
	   ipv6_valid:1,
	   src_port_range_reg_valid:1,
	   dst_port_range_reg_valid:1;
	unsigned int counter_index;
	u16 policer_index;
	u8 src_port_range_reg_index;
	u8 dst_port_range_reg_index;
	struct {
		u32 prev_val;
		enum mlxsw_sp_acl_mangle_field prev_field;
	} ipv6;
};
/* …（同文件无关代码省略）… */
struct mlxsw_sp_acl_rule_info *
mlxsw_sp_acl_rulei_create(struct mlxsw_sp_acl *acl,
			  struct mlxsw_afa_block *afa_block);
void mlxsw_sp_acl_rulei_destroy(struct mlxsw_sp *mlxsw_sp,
				struct mlxsw_sp_acl_rule_info *rulei);
int mlxsw_sp_acl_rulei_commit(struct mlxsw_sp_acl_rule_info *rulei);
