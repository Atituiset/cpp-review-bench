// AUTO-DRAFT from torvalds/linux PR #bf2325084583daaea91084b25069f121d5d8832b
work_data->client = ctx->client;
  // <<< BUG ANCHOR
	ret = atmel_i2c_init_ecdh_cmd(&work_data->cmd, req->src);
	if (ret)
		goto free_work_data;

	atmel_i2c_enqueue(work_data, atmel_ecdh_done, req);

	return -EINPROGRESS;

free_work_data:
	kfree(work_data);
	return ret;
}

static struct i2c_client *atmel_ecc_i2c_client_alloc(void)

static const struct of_device_id atmel_ecc_dt_ids[] = {
	{ .compatible = "atmel,atecc508a", },
	{ .compatible = "atmel,atecc608b", },
	{ }
};
MODULE_DEVICE_TABLE(of, atmel_ecc_dt_ids);

static const struct i2c_device_id atmel_ecc_id[] = {
	{ .name = "atecc508a" },
	{ .name = "atecc608b" },
	{ }
};
