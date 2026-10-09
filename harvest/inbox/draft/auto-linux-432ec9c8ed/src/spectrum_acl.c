// AUTO-DRAFT from torvalds/linux PR #ff6f5157e034f81c2d6482259fc656cd0fe5a8ff
	return ERR_PTR(err);
}
  // <<< BUG ANCHOR
void mlxsw_sp_acl_rulei_destroy(struct mlxsw_sp *mlxsw_sp,
				struct mlxsw_sp_acl_rule_info *rulei)
{
	if (rulei->action_created)
		mlxsw_afa_block_destroy(rulei->act_block);
/* …（同文件无关代码省略）… */
	if (rulei->dst_port_range_reg_valid)
		mlxsw_sp_port_range_reg_put(mlxsw_sp,
					    rulei->dst_port_range_reg_index);
	kfree(rulei);
}
