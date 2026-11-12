# auto-linux-fdf4216d3a

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#0eaee8fc9305b4ae8e75e68c30e7e93b6b367234](https://github.com/torvalds/linux/commit/0eaee8fc9305b4ae8e75e68c30e7e93b6b367234) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-12 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 281 |
| 编译错误数（gcc syntax-only） | 537（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #0eaee8fc9305b4ae8e75e68c30e7e93b6b367234 (https://github.com/torvalds/linux/commit/0eaee8fc9305b4ae8e75e68c30e7e93b6b367234)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 1930；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 0eaee8fc9305b4ae8e75e68c30e7e93b6b367234 Merge tag 'v7.4-p2' of git://git.kernel.org/pub/scm/linux/kernel/git/herbert/crypto-2.6 :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1916,6 +1916,8 @@ static int sev_ioctl_do_reset(struct sev_issue_cmd *argp, bool writable)
 	if (!writable)
 		return -EPERM;
 
+	mutex_lock(&sev_cmd_mutex);
+
 	/*
 	 * The SEV spec requires that FACTORY_RESET must be issued in
 	 * UNINIT state. Before we go further lets check if any guest is
@@ -1927,18 +1929,24 @@ static int sev_ioctl_do_reset(struct sev_issue_cmd *argp, bool writable)
 	 */
 	rc = sev_get_platform_state(&state, &argp->error);
 	if (rc)
-		return rc;
+		goto out_unlock;
 
-	if (state == SEV_STATE_WORKING)
-		return -EBUSY;
+	if (state == SEV_STATE_WORKING) {
+		rc = -EBUSY;
+		goto out_unlock;
+	}
 
 	if (state == SEV_STATE_INIT) {
 		rc = __sev_platform_shutdown_locked(&argp->error);
 		if (rc)
-			return rc;
+			goto out_unlock;
 	}
 
-	return __sev_do_cmd_locked(SEV_CMD_FACTORY_RESET, NULL, &argp->error);
+	rc = __sev_do_cmd_locked(SEV_CMD_FACTORY_RESET, NULL, &argp->error);
+
+out_unlock:
+	mutex_unlock(&sev_cmd_mutex);
+	return rc;
 }
 
 static int sev_ioctl_do_platform_status(struct sev_issue_cmd *argp)
@@ -1948,7 +1956,9 @@ static int sev_ioctl_do_platform_status(struct sev_issue_cmd *argp)
 
 	memset(&data, 0, sizeof(data));
 
+	mutex_lock(&sev_cmd_mutex);
 	ret = __sev_do_cmd_locked(SEV_CMD_PLATFORM_STATUS, &data, &argp->error);
+	mutex_unlock(&sev_cmd_mutex);
 	if (ret)
 		return ret;
 
@@ -1967,17 +1977,22 @@ static int sev_ioctl_do_pek_pdh_gen(int cmd, struct sev_issue_cmd *argp, bool wr
 	if (!writable)
 		return -EPERM;
 
+	mutex_lock(&sev_cmd_mutex);
+
 	if (sev->sev_plat_status.state == SEV_STATE_UNINIT) {
 		rc = sev_move_to_init_state(argp, &shutdown_required);
 		if (rc)
-			return rc;
+			goto out_unlock;
 	}
 
 	rc = __sev_do_cmd_locked(cmd, NULL, &argp->error);
 
 	if (shutdown_required)
 		__sev_firmware_shutdown(sev, false);
 
+out_unlock:
+	mutex_unlock(&sev_cmd_mutex);
+
 	return rc;
 }
 
@@ -2016,14 +2031,24 @@ static int sev_ioctl_do_pek_csr(struct sev_issue_cmd *argp, bool writable)
 	data.len = input.length;
 
 cmd:
+	mutex_lock(&sev_cmd_mutex);
+
 	if (sev->sev_plat_status.state == SEV_STATE_UNINIT) {
 		ret = sev_move_to_init_state(argp, &shutdown_required);
-		if (ret)
+		if (ret) {
+			mutex_unlock(&sev_cmd_mutex);
+			/* No shutdown is needed if init fails */
 			goto e_free_blob;
+		}
 	}
 
 	ret = __sev_do_cmd_locked(SEV_CMD_PEK_CSR, &data, &argp->error);
 
+	if (shutdown_required)
+		__sev_firmware_shutdown(sev, false);
+
+	mutex_unlock(&sev_cmd_mutex);
+
 	/*
 	 * Firmware will returns the length of the CSR blob (either the minimum
 	 * required length or the actual length written), return it to the user.
@@ -2044,9 +2069,6 @@ static int sev_ioctl_do_pek_csr(struct sev_issue_cmd *argp, bool writable)
 	}
 
 e_free_blob:
-	if (shutdown_required)
-		__sev_firmware_shutdown(sev, false);
-
 	kfree(blob);
 	return ret;
 }
@@ -2319,19 +2341,26 @@ static int sev_ioctl_do_pek_import(struct sev_issue_cmd *argp, bool writable)
 	data.oca_cert_address = __psp_pa(oca_blob);
 	data.oca_cert_len = input.oca_cert_len;
 
+	mutex_lock(&sev_cmd_mutex);
+
 	/* If platform is not in INIT state then transition it to INIT */
 	if (sev->sev_plat_status.state != SEV_STATE_INIT) {
 		ret = sev_move_to_init_state(argp, &shutdown_required);
-		if (ret)
+		if (ret) {
+			mutex_unlock(&sev_cmd_mutex);
+			/* No shutdown is needed if init fails */
 			goto e_free_oca;
+		}
 	}
 
 	ret = __sev_do_cmd_locked(SEV_CMD_PEK_CERT_IMPORT, &data, &argp->error);
 
-e_free_oca:
 	if (shutdown_required)
 		__sev_firmware_shutdown(sev, false);
 
+	mutex_unlock(&sev_cmd_mutex);
+
+e_free_oca:
 	kfree(oca_blob);
 e_free_pek:
 	kfree(pek_blob);
@@ -2374,7 +2403,9 @@ static int sev_ioctl_do_get_id2(struct sev_issue_cmd *argp)
 		data.len = 0;
 	}
 
+	mutex_lock(&sev_cmd_mutex);
 	ret = __sev_do_cmd_locked(SEV_CMD_GET_ID, &data, &argp->error);
+	mutex_unlock(&sev_cmd_mutex);
 
 	/*
 	 * Firmware will return the length of the ID value (either the minimum
@@ -2432,7 +2463,9 @@ static int sev_ioctl_do_get_id(struc
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__free_pages`
- 外部函数：`__phys_to_pfn`
- 外部函数：`__psp_pa`
- 外部函数：`__sme_clr`
- 外部函数：`__snp_leak_pages`
- 外部函数：`__va`
- 外部函数：`alloc_page`
- 外部函数：`alloc_pages`
- 外部函数：`amd_iommu_snp_disable`
- 外部函数：`atomic_notifier_chain_unregister`
- 外部函数：`blob`
- 外部函数：`blobs`
- 外部函数：`clflush_cache_range`
- 外部函数：`copy_from_user`
- 外部函数：`copy_to_user`
- 外部函数：`dev_dbg`
- 外部函数：`dev_err`
- 外部函数：`dev_info`
- 外部函数：`dev_warn`
- 外部函数：`failed`
- 外部函数：`filp_close`
- 外部函数：`filp_open`
- 外部函数：`get_order`
- 外部函数：`ioread32`
- 外部函数：`iowrite32`
- 外部函数：`irqs_disabled`
- 外部函数：`kernel_read`
- 外部函数：`kernel_write`
- 外部函数：`kfree`
- 外部函数：`list_del`
- 外部函数：`list_empty`
- 外部函数：`lockdep_assert_held`
- 外部函数：`lower_32_bits`
- 外部函数：`memcpy`
- 外部函数：`memdup_user`
- 外部函数：`memset`
- 外部函数：`mutex_lock`
- 外部函数：`mutex_unlock`
- 外部函数：`page_address`
- 外部函数：`page_to_pfn`
- 外部函数：`page_to_virt`
- 外部函数：`pr_warn`
- 外部函数：`print_hex_dump_debug`
- 外部函数：`rmp_make_private`
- 外部函数：`rmp_make_shared`
- 外部函数：`scoped_with_creds`
- 外部函数：`sev_tio_cmd_buffer_len`
- 外部函数：`snp_leak_pages`
- 外部函数：`snp_shutdown`
- 外部函数：`u64_to_user_ptr`
- 外部函数：`udelay`
- 外部函数：`upper_32_bits`
- 外部函数：`value`
- 外部函数：`vfs_fsync`
- 外部函数：`virt_to_page`
- 外部函数：`wait_event_timeout`
- 外部函数：`wbinvd`
- 外部函数：`wbinvd_on_all_cpus`
- 大写宏：`ALIGN_DOWN`
- 大写宏：`ALLOCATED`
- 大写宏：`API`
- 大写宏：`CLASS`
- 大写宏：`CPU`
- 大写宏：`CSR`
- 大写宏：`DF_FLUSH`
- 大写宏：`DUMP_PREFIX_OFFSET`
- 大写宏：`EBUSY`
- 大写宏：`EFAULT`
- 大写宏：`EINVAL`
- 大写宏：`EIO`
- 大写宏：`ENODEV`
- 大写宏：`ENOENT`
- 大写宏：`ENOMEM`
- 大写宏：`EOPNOTSUPP`
- 大写宏：`EPERM`
- 大写宏：`ERR_PTR`
- 大写宏：`ETIMEDOUT`
- 大写宏：`FACTORY_RESET`
- 大写宏：`FIELD_GET`
- 大写宏：`FIELD_PREP`
- 大写宏：`GFP_KERNEL`
- 大写宏：`GFP_KERNEL_ACCOUNT`
- 大写宏：`GLOBAL_ROOT_UID`
- 大写宏：`HV_FIXED`
- 大写宏：`INIT`
- 大写宏：`INIT_EX`
- 大写宏：`INVALID_PAGE_STATE`
- 大写宏：`IOMMU`
- 大写宏：`IOMMU_SNP_SHUTDOWN`
- 大写宏：`IS_ERR`
- 大写宏：`NMI`
- 大写宏：`NULL`
- 大写宏：`O_CREAT`
- 大写宏：`O_RDONLY`
- 大写宏：`O_WRONLY`
- 大写宏：`PAGE_ALIGN`
- 大写宏：`PAGE_SHIFT`
- 大写宏：`PAGE_SIZE`
- 大写宏：`PDH_GEN`
- 大写宏：`PEK_CERT_IMPORT`
- 大写宏：`PEK_GEN`
- 大写宏：`PG_LEVEL_4K`
- 大写宏：`PLATFORM_RESET`
- 大写宏：`PSP`
- 大写宏：`PSP_CMDRESP_RESP`
- 大写宏：`PSP_CMDRESP_STS`
- 大写宏：`PTR_ERR`
- 大写宏：`RCU`
- 大写宏：`RMP`
- 大写宏：`SECURE_DATA_INVALID`
- 大写宏：`SEV`
- 大写宏：`SEV_CMDRESP_CMD`
- 大写宏：`SEV_CMDRESP_IOC`
- 大写宏：`SEV_CMD_ACTIVATE`
- 大写宏：`SEV_CMD_ATTESTATION_REPORT`
- 大写宏：`SEV_CMD_DBG_DECRYPT`
- 大写宏：`SEV_CMD_DBG_ENCRYPT`
- 大写宏：`SEV_CMD_DEACTIVATE`
- 大写宏：`SEV_CMD_DECOMMISSION`
- 大写宏：`SEV_CMD_DF_FLUSH`
- 大写宏：`SEV_CMD_DOWNLOAD_FIRMWARE`
- 大写宏：`SEV_CMD_FACTORY_RESET`
- 大写宏：`SEV_CMD_GET_ID`
- 大写宏：`SEV_CMD_GUEST_STATUS`
- 大写宏：`SEV_CMD_INIT`
- 大写宏：`SEV_CMD_INIT_EX`
- 大写宏：`SEV_CMD_LAUNCH_FINISH`
- 大写宏：`SEV_CMD_LAUNCH_MEASURE`
- 大写宏：`SEV_CMD_LAUNCH_START`
- 大写宏：`SEV_CMD_LAUNCH_UPDATE_DATA`
- 大写宏：`SEV_CMD_LAUNCH_UPDATE_SECRET`
- 大写宏：`SEV_CMD_LAUNCH_UPDATE_VMSA`
- 大写宏：`SEV_CMD_PDH_CERT_EXPORT`
- 大写宏：`SEV_CMD_PDH_GEN`
- 大写宏：`SEV_CMD_PEK_CERT_IMPORT`
- 大写宏：`SEV_CMD_PEK_CSR`
- 大写宏：`SEV_CMD_PEK_GEN`
- 大写宏：`SEV_CMD_PLATFORM_STATUS`
- 大写宏：`SEV_CMD_RECEIVE_FINISH`
- 大写宏：`SEV_CMD_RECEIVE_START`
- 大写宏：`SEV_CMD_RECEIVE_UPDATE_DATA`
- 大写宏：`SEV_CMD_RECEIVE_UPDATE_VMSA`
- 大写宏：`SEV_CMD_SEND_CANCEL`
- 大写宏：`SEV_CMD_SEND_FINISH`
- 大写宏：`SEV_CMD_SEND_START`
- 大写宏：`SEV_CMD_SEND_UPDATE_DATA`
- 大写宏：`SEV_CMD_SEND_UPDATE_VMSA`
- 大写宏：`SEV_CMD_SHUTDOWN`
- 大写宏：`SEV_CMD_SNP_ACTIVATE`
- 大写宏：`SEV_CMD_SNP_COMMIT`
- 大写宏：`SEV_CMD_SNP_CONFIG`
- 大写宏：`SEV_CMD_SNP_DBG_DECRYPT`
- 大写宏：`SEV_CMD_SNP_DBG_ENCRYPT`
- 大写宏：`SEV_CMD_SNP_DECOMMISSION`
- 大写宏：`SEV_CMD_SNP_DF_FLUSH`
- 大写宏：`SEV_CMD_SNP_FEATURE_INFO`
- 大写宏：`SEV_CMD_SNP_GCTX_CREATE`
- 大写宏：`SEV_CMD_SNP_GUEST_REQUEST`
- 大写宏：`SEV_CMD_SNP_GUEST_STATUS`
- 大写宏：`SEV_CMD_SNP_INIT`
- 大写宏：`SEV_CMD_SNP_INIT_EX`
- 大写宏：`SEV_CMD_SNP_LAUNCH_FINISH`
- 大写宏：`SEV_CMD_SNP_LAUNCH_START`
- 大写宏：`SEV_CMD_SNP_LAUNCH_UPDATE`
- 大写宏：`SEV_CMD_SNP_PAGE_RECLAIM`
- 大写宏：`SEV_CMD_SNP_PAGE_UNSMASH`
- 大写宏：`SEV_CMD_SNP_PLATFORM_STATUS`
- 大写宏：`SEV_CMD_SNP_SHUTDOWN_EX`
- 大写宏：`SEV_CMD_SNP_VERIFY_MITIGATION`
- 大写宏：`SEV_CMD_SNP_VLEK_LOAD`
- 大写宏：`SEV_FACTORY_RESET`
- 大写宏：`SEV_FW_BLOB_MAX_SIZE`
- 大写宏：`SEV_INIT_FLAGS_SEV_ES`
- 大写宏：`SEV_MAX`
- 大写宏：`SEV_RET_DFFLUSH_REQUIRED`
- 大写宏：`SEV_RET_NO_FW_CALL`
- 大写宏：`SEV_RET_SECURE_DATA_INVALID`
- 大写宏：`SEV_STATE_INIT`
- 大写宏：`SEV_STATE_UNINIT`
- 大写宏：`SEV_STATE_WORKING`
- 大写宏：`SHUTDOWN`
- 大写宏：`SNP`
- 大写宏：`SNP_PLATFORM_STATUS`
- 大写宏：`SNP_SHUTDOWN_EX`
- 大写宏：`SNP_X86_SHUTDOWN_SUPPORTED`
- 大写宏：`SPI`
- 大写宏：`TMR`
- 大写宏：`UNINIT`
- 大写宏：`USEC_PER_SEC`
- 大写宏：`WARN_ON_ONCE`
- 外部类型：`Additional`
- 外部类型：`After`
- 外部类型：`Allocate`
- 外部类型：`Before`
- 外部类型：`But`
- 外部类型：`CPUs`
- 外部类型：`Commands`
- 外部类型：`Copy`
- 外部类型：`Do`
- 外部类型：`Ensure`
- 外部类型：`Error`
- 外部类型：`Failed`
- 外部类型：`Firmware`
- 外部类型：`Get`
- 外部类型：`Hence`
- 外部类型：`Hypervisor`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Initialization`
- 外部类型：`List`
- 外部类型：`Must`
- 外部类型：`Obtain`
- 外部类型：`Only`
- 外部类型：`Pages`
- 外部类型：`Poll`
- 外部类型：`Prepare`
- 外部类型：`Print`
- 外部类型：`Reclaim`
- 外部类型：`Reset`
- 外部类型：`Restore`
- 外部类型：`Retrying`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Transition`
- 外部类型：`Try`
- 外部类型：`Use`
- 外部类型：`file`
- 外部类型：`gfp_t`
- 外部类型：`list_head`
- 外部类型：`loff_t`
- 外部类型：`page`
- 外部类型：`psp_device`
- 外部类型：`sev_data_activate`
- 外部类型：`sev_data_attestation_report`
- 外部类型：`sev_data_dbg`
- 外部类型：`sev_data_deactivate`
- 外部类型：`sev_data_decommission`
- 外部类型：`sev_data_download_firmware`
- 外部类型：`sev_data_get_id`
- 外部类型：`sev_data_guest_status`
- 外部类型：`sev_data_init`
- 外部类型：`sev_data_init_ex`
- 外部类型：`sev_data_launch_finish`
- 外部类型：`sev_data_launch_measure`
- 外部类型：`sev_data_launch_secret`
- 外部类型：`sev_data_launch_start`
- 外部类型：`sev_data_launch_update_data`
- 外部类型：`sev_data_launch_update_vmsa`
- 外部类型：`sev_data_pdh_cert_export`
- 外部类型：`sev_data_pek_cert_import`
- 外部类型：`sev_data_pek_csr`
- 外部类型：`sev_data_receive_finish`
- 外部类型：`sev_data_receive_start`
- 外部类型：`sev_data_receive_update_data`
- 外部类型：`sev_data_receive_update_vmsa`
- 外部类型：`sev_data_send_cancel`
- 外部类型：`sev_data_send_finish`
- 外部类型：`sev_data_send_start`
- 外部类型：`sev_data_send_update_data`
- 外部类型：`sev_data_send_update_vmsa`
- 外部类型：`sev_data_snp_activate`
- 外部类型：`sev_data_snp_addr`
- 外部类型：`sev_data_snp_commit`
- 外部类型：`sev_data_snp_dbg`
- 外部类型：`sev_data_snp_feature_info`
- 外部类型：`sev_data_snp_guest_request`
- 外部类型：`sev_data_snp_guest_status`
- 外部类型：`sev_data_snp_init_ex`
- 外部类型：`sev_data_snp_launch_finish`
- 外部类型：`sev_data_snp_launch_start`
- 外部类型：`sev_data_snp_launch_update`
- 外部类型：`sev_data_snp_page_reclaim`
- 外部类型：`sev_data_snp_page_unsmash`
- 外部类型：`sev_data_snp_shutdown_ex`
- 外部类型：`sev_data_snp_verify_mitigation`
- 外部类型：`sev_device`
- 外部类型：`sev_issue_cmd`
- 外部类型：`sev_snp_tcb_version_turin`
- 外部类型：`sev_user_data_snp_config`
- 外部类型：`sev_user_data_snp_status`
- 外部类型：`sev_user_data_snp_vlek_load`
- 外部类型：`sev_user_data_snp_wrapped_vlek_hashstick`
- 外部类型：`sev_user_data_status`
- 外部类型：`ssize_t`
- 外部类型：`umode_t`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。
- **依赖重（dep_count≥10）**：可考虑只做 PR/diff 形态评审，不做独立 case。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-linux-fdf4216d3a` → 本草稿移入 `cases/defect/auto-linux-fdf4216d3a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
