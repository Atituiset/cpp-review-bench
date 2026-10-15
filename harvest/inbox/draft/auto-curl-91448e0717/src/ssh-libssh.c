// AUTO-DRAFT from curl/curl PR #2144
if(rc != 0 && !sshc->acceptfail) {
        Curl_safefree(sshc->quote_path1);
        Curl_safefree(sshc->quote_path2);
        err = sftp_get_error(sshc->sftp_session);
        failf(data, "Attempt to set SFTP stats failed: %s",
              ssh_get_error(sshc->ssh_session));
        state(conn, SSH_SFTP_CLOSE);
      if(rc != 0 && !sshc->acceptfail) {
        Curl_safefree(sshc->quote_path1);
        Curl_safefree(sshc->quote_path2);
        err = sftp_get_error(sshc->sftp_session);
        failf(data, "symlink command failed: %s",
              ssh_get_error(sshc->ssh_session));
        state(conn, SSH_SFTP_CLOSE);
                      (mode_t)data->set.new_directory_perms);
      if(rc != 0 && !sshc->acceptfail) {
        Curl_safefree(sshc->quote_path1);
        err = sftp_get_error(sshc->sftp_session);
        failf(data, "mkdir command failed: %s",
              ssh_get_error(sshc->ssh_session));
        state(conn, SSH_SFTP_CLOSE);
      if(rc != 0 && !sshc->acceptfail) {
        Curl_safefree(sshc->quote_path1);
        Curl_safefree(sshc->quote_path2);
        err = sftp_get_error(sshc->sftp_session);
        failf(data, "rename command failed: %s",
              ssh_get_error(sshc->ssh_session));
        state(conn, SSH_SFTP_CLOSE);
      rc = sftp_rmdir(sshc->sftp_session, sshc->quote_path1);
      if(rc != 0 && !sshc->acceptfail) {
        Curl_safefree(sshc->quote_path1);
        err = sftp_get_error(sshc->sftp_session);
        failf(data, "rmdir command failed: %s",
              ssh_get_error(sshc->ssh_session));
        state(conn, SSH_SFTP_CLOSE);
      rc = sftp_unlink(sshc->sftp_session, sshc->quote_path1);
      if(rc != 0 && !sshc->acceptfail) {
        Curl_safefree(sshc->quote_path1);
        err = sftp_get_error(sshc->sftp_session);
        failf(data, "rm command failed: %s",
              ssh_get_error(sshc->ssh_session));
        state(conn, SSH_SFTP_CLOSE);
      sftp_statvfs_t statvfs;
  // <<< BUG ANCHOR
      statvfs = sftp_statvfs(sshc->sftp_session,
