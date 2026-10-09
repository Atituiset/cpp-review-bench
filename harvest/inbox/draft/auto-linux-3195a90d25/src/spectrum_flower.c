// AUTO-DRAFT from torvalds/linux PR #ff6f5157e034f81c2d6482259fc656cd0fe5a8ff
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
static int mlxsw_sp_policer_validate(const struct flow_action *action,
				     const struct flow_action_entry *act,
				     struct netlink_ext_ack *extack)
{
	if (act->police.exceed.act_id != FLOW_ACTION_DROP) {
		NL_SET_ERR_MSG_MOD(extack,
				   "Offload not supported when exceed action is not drop");
		return -EOPNOTSUPP;
	}

	if (act->police.notexceed.act_id != FLOW_ACTION_PIPE &&
	    act->police.notexceed.act_id != FLOW_ACTION_ACCEPT) {
		NL_SET_ERR_MSG_MOD(extack,
				   "Offload not supported when conform action is not pipe or ok");
		return -EOPNOTSUPP;
	}

	if (act->police.notexceed.act_id == FLOW_ACTION_ACCEPT &&
	    !flow_action_is_last_entry(action, act)) {
		NL_SET_ERR_MSG_MOD(extack,
				   "Offload not supported when conform action is ok, but action is not last");
		return -EOPNOTSUPP;
	}

	if (act->police.peakrate_bytes_ps ||
	    act->police.avrate || act->police.overhead) {
		NL_SET_ERR_MSG_MOD(extack,
				   "Offload not supported when peakrate/avrate/overhead is configured");
		return -EOPNOTSUPP;
	}

	if (act->police.rate_pkt_ps) {
		NL_SET_ERR_MSG_MOD(extack,
				   "QoS offload not support packets per second");
		return -EOPNOTSUPP;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static int mlxsw_sp_flower_parse_actions(struct mlxsw_sp *mlxsw_sp,
					 struct mlxsw_sp_flow_block *block,
					 struct mlxsw_sp_acl_rule_info *rulei,
					 struct flow_action *flow_action,
					 struct netlink_ext_ack *extack)
{
	const struct flow_action_entry *act;
	int mirror_act_count = 0;
	int police_act_count = 0;
	int sample_act_count = 0;
	int err, i;

	if (!flow_action_has_entries(flow_action))
		return 0;
	if (!flow_action_mixed_hw_stats_check(flow_action, extack))
		return -EOPNOTSUPP;

	act = flow_action_first_entry_get(flow_action);
	if (act->hw_stats & FLOW_ACTION_HW_STATS_DISABLED) {
		/* Nothing to do */
	} else if (act->hw_stats & FLOW_ACTION_HW_STATS_IMMEDIATE) {
		/* Count action is inserted first */
		err = mlxsw_sp_acl_rulei_act_count(mlxsw_sp, rulei, extack);
		if (err)
			return err;
	} else {
		NL_SET_ERR_MSG_MOD(extack, "Unsupported action HW stats type");
		return -EOPNOTSUPP;
	}

	flow_action_for_each(i, act, flow_action) {
		switch (act->id) {
		case FLOW_ACTION_ACCEPT:
			err = mlxsw_sp_acl_rulei_act_terminate(rulei);
			if (err) {
				NL_SET_ERR_MSG_MOD(extack, "Cannot append terminate action");
				return err;
			}
			break;
		case FLOW_ACTION_DROP: {
			bool ingress;

			if (mlxsw_sp_flow_block_is_mixed_bound(block)) {
				NL_SET_ERR_MSG_MOD(extack, "Drop action is not supported when block is bound to ingress and egress");
				return -EOPNOTSUPP;
			}
			ingress = mlxsw_sp_flow_block_is_ingress_bound(block);
			err = mlxsw_sp_acl_rulei_act_drop(rulei, ingress,
							  act->user_cookie, extack);
			if (err) {
				NL_SET_ERR_MSG_MOD(extack, "Cannot append drop action");
				return err;
			}

			/* Forbid block with this rulei to be bound
			 * to ingress/egress in future. Ingress rule is
			 * a blocker for egress and vice versa.
			 */
			if (ingress)
				rulei->egress_bind_blocker = 1;
			else
				rulei->ingress_bind_blocker = 1;
			}
			break;
		case FLOW_ACTION_TRAP:
			err = mlxsw_sp_acl_rulei_act_trap(rulei);
			if (err) {
				NL_SET_ERR_MSG_MOD(extack, "Cannot append trap action");
				return err;
			}
			break;
		case FLOW_ACTION_GOTO: {
			u32 chain_index = act->chain_index;
			struct mlxsw_sp_acl_ruleset *ruleset;
			u16 group_id;

			ruleset = mlxsw_sp_acl_ruleset_lookup(mlxsw_sp, block,
							      chain_index,
							      MLXSW_SP_ACL_PROFILE_FLOWER);
			if (IS_ERR(ruleset))
				return PTR_ERR(ruleset);

			group_id = mlxsw_sp_acl_ruleset_group_id(ruleset);
			err = mlxsw_sp_acl_rulei_act_jump(rulei, group_id);
			if (err) {
				NL_SET_ERR_MSG_MOD(extack, "Cannot append jump action");
				return err;
			}
			}
			break;
		case FLOW_ACTION_REDIRECT: {
			struct net_device *out_dev;
			struct mlxsw_sp_fid *fid;
			u16 fid_index;

			if (mlxsw_sp_flow_block_is_egress_bound(block)) {
				NL_SET_ERR_MSG_MOD(extack, "Redirect action is not supported on egress");
				return -EOPNOTSUPP;
			}

			/* Forbid block with this rulei to be bound
			 * to egress in future.
			 */
			rulei->egress_bind_blocker = 1;

			/* Ignore learning and security lookup as redirection
			 * using ingress filters happens before the bridge.
			 */
			err = mlxsw_sp_acl_rulei_act_ignore(mlxsw_sp, rulei,
							    true, true);
			if (err) {
				NL_SET_ERR_MSG_MOD(extack, "Cannot append ignore action");
				return err;
			}

			fid = mlxsw_sp_acl_dummy_fid(mlxsw_sp);
			fid_index = mlxsw_sp_fid_index(fid);
			err = mlxsw_sp_acl_rulei_act_fid_set(mlxsw_sp, rulei,
							     fid_index, extack);
			if (err)
				return err;

			out_dev = act->dev;
			err = mlxsw_sp_acl_rulei_act_fwd(mlxsw_sp, rulei,
							 out_dev, extack);
			if (err)
				return err;
			}
			break;
		case FLOW_ACTION_MIRRED: {
			struct net_device *out_dev = act->dev;

			if (mirror_act_count++) {
				NL_SET_ERR_MSG_MOD(extack, "Multiple mirror actions per rule are not supported");
				return -EOPNOTSUPP;
			}

			if (sample_act_count) {
				NL_SET_ERR_MSG_MOD(extack, "Mirror action after sample action is not supported");
				return -EOPNOTSUPP;
			}

			err = mlxsw_sp_acl_rulei_act_mirror(mlxsw_sp, rulei,
							    block, out_dev,
							    extack);
			if (err)
				return err;
			}
			break;
		case FLOW_ACTION_VLAN_MANGLE: {
			u16 proto = be16_to_cpu(act->vlan.proto);
			u8 prio = act->vlan.prio;
			u16 vid = act->vlan.vid;

			err = mlxsw_sp_acl_rulei_act_vlan(mlxsw_sp, rulei,
							  act->id, vid,
							  proto, prio, extack);
			if (err)
				return err;
			break;
			}
		case FLOW_ACTION_PRIORITY:
			err = mlxsw_sp_acl_rulei_act_priority(mlxsw_sp, rulei,
							      act->priority,
							      extack);
			if (err)
				return err;
			break;
		case FLOW_ACTION_MANGLE: {
			enum flow_action_mangle_
