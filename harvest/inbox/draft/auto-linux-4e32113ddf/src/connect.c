// AUTO-DRAFT from torvalds/linux PR #89a312991dc6e638a36adc43ccb91dbc25504c04
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
static int
cifs_set_cifscreds(struct smb3_fs_context *ctx, struct cifs_ses *ses)
{
	int rc = 0;
	int is_domain = 0;
	const char *delim, *payload;
	size_t desc_sz;
	char *desc;
	ssize_t len;
	struct key *key;
	struct TCP_Server_Info *server = ses->server;
	struct sockaddr_in *sa;
	struct sockaddr_in6 *sa6;
	const struct user_key_payload *upayload;

	/* "cifs:a:" and "cifs:d:" are the same length; +1 for NUL terminator */
	desc_sz = strlen("cifs:a:") + CIFS_MAX_DOMAINNAME_LEN + 1;
	desc = kmalloc(desc_sz, GFP_KERNEL);
	if (!desc)
		return -ENOMEM;

	/* try to find an address key first */
	switch (server->dstaddr.ss_family) {
	case AF_INET:
		sa = (struct sockaddr_in *)&server->dstaddr;
		snprintf(desc, desc_sz, "cifs:a:%pI4", &sa->sin_addr.s_addr);
		break;
	case AF_INET6:
		sa6 = (struct sockaddr_in6 *)&server->dstaddr;
		snprintf(desc, desc_sz, "cifs:a:%pI6c", &sa6->sin6_addr.s6_addr);
		break;
	default:
		cifs_dbg(FYI, "Bad ss_family (%hu)\n",
			 server->dstaddr.ss_family);
		rc = -EINVAL;
		goto out_err;
	}

	cifs_dbg(FYI, "%s: desc=%s\n", __func__, desc);
	key = request_key(&key_type_logon, desc, "");
	if (IS_ERR(key)) {
		if (!ses->domainName) {
			cifs_dbg(FYI, "domainName is NULL\n");
			rc = PTR_ERR(key);
			goto out_err;
		}

		/* didn't work, try to find a domain key */
		snprintf(desc, desc_sz, "cifs:d:%s", ses->domainName);
		cifs_dbg(FYI, "%s: desc=%s\n", __func__, desc);
		key = request_key(&key_type_logon, desc, "");
		if (IS_ERR(key)) {
			rc = PTR_ERR(key);
			goto out_err;
		}
		is_domain = 1;
	}

	down_read(&key->sem);
	upayload = user_key_payload_locked(key);
	if (IS_ERR_OR_NULL(upayload)) {
		rc = upayload ? PTR_ERR(upayload) : -EINVAL;
		goto out_key_put;
	}

	/* find first : in payload */
	payload = upayload->data;
	delim = strnchr(payload, upayload->datalen, ':');
	if (!delim) {
		cifs_dbg(FYI, "Unable to find ':' in payload (datalen=%d)\n",
			 upayload->datalen);
		rc = -EINVAL;
		goto out_key_put;
	}

	len = delim - payload;
	if (len > CIFS_MAX_USERNAME_LEN || len <= 0) {
		cifs_dbg(FYI, "Bad value from username search (len=%zd)\n",
			 len);
		rc = -EINVAL;
		goto out_key_put;
	}

	ctx->username = kstrndup(payload, len, GFP_KERNEL);
	if (!ctx->username) {
		cifs_dbg(FYI, "Unable to allocate %zd bytes for username\n",
			 len);
		rc = -ENOMEM;
		goto out_key_put;
	}
	cifs_dbg(FYI, "%s: username=%s\n", __func__, ctx->username);

	len = key->datalen - (len + 1);
	if (len > CIFS_MAX_PASSWORD_LEN || len <= 0) {
		cifs_dbg(FYI, "Bad len for password search (len=%zd)\n", len);
		rc = -EINVAL;
		kfree(ctx->username);
		ctx->username = NULL;
		goto out_key_put;
	}

	++delim;
	/* BB consider adding support for password2 (Key Rotation) for multiuser in future */
	ctx->password = kstrndup(delim, len, GFP_KERNEL);
	if (!ctx->password) {
		cifs_dbg(FYI, "Unable to allocate %zd bytes for password\n",
			 len);
		rc = -ENOMEM;
		kfree(ctx->username);
		ctx->username = NULL;
		goto out_key_put;
	}

	/*
	 * If we have a domain key then we must set the domainName in the
	 * for the request.
	 */
	if (is_domain && ses->domainName) {
		ctx->domainname = kstrdup(ses->domainName, GFP_KERNEL);
		if (!ctx->domainname) {
			cifs_dbg(FYI, "Unable to allocate %zd bytes for domain\n",
				 len);
			rc = -ENOMEM;
			kfree(ctx->username);
			ctx->username = NULL;
			kfree_sensitive(ctx->password);
			/* no need to free ctx->password2 since not allocated in this path */
			ctx->password = NULL;
			goto out_key_put;
		}
	}

	strscpy(ctx->workstation_name, ses->workstation_name, sizeof(ctx->workstation_name));

out_key_put:
	up_read(&key->sem);
	key_put(key);
out_err:
	kfree(desc);
	cifs_dbg(FYI, "%s: returning %d\n", __func__, rc);
	return rc;
}
/* …（同文件无关代码省略）… */
	return rc;
}

static int
cifs_set_vol_auth(struct smb3_fs_context *ctx, struct cifs_ses *ses)
{
	ctx->sectype = ses->sectype;

	/* krb5 is special, since we don't need username or pw */
	if (ctx->sectype == Kerberos)
		return 0;

	return cifs_set_cifscreds(ctx, ses);
}
/* …（同文件无关代码省略）… */
	ctx->dfs_root_ses = master_tcon->ses->dfs_root_ses;
	ctx->unicode = master_tcon->ses->unicode;

	rc = cifs_set_vol_auth(ctx, master_tcon->ses);
	if (rc) {
		tcon = ERR_PTR(rc);
		goto out;
