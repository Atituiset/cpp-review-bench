// AUTO-DRAFT from curl/curl PR #1727
dir) this then allows for a second try to CWD to it */
    ftpc->count3 = (conn->data->set.ftp_create_missing_dirs==2)?1:0;
  // <<< BUG ANCHOR
    if(conn->bits.reuse && ftpc->entrypath) {
      /* This is a re-used connection. Since we change directory to where the
         transfer is taking place, we must first get back to the original dir
         where we ended up after login: */
      ftpc->count1 = 0; /* we count this as the first path, then we add one
                          for all upcoming ones in the ftp->dirs[] array */
      PPSENDF(&conn->proto.ftpc.pp, "CWD %s", ftpc->entrypath);
      state(conn, FTP_CWD);
    }
    else {
      if(ftpc->dirdepth) {
        ftpc->count1 = 1;
        /* issue the first CWD, the rest is sent when the CWD responses are
           received... */
        PPSENDF(&conn->proto.ftpc.pp, "CWD %s", ftpc->dirs[ftpc->count1 -1]);
        state(conn, FTP_CWD);
      }
      else {
      if(ftpcode/100 != 2) {
        /* failure to CWD there */
        if(conn->data->set.ftp_create_missing_dirs &&
           ftpc->count1 && !ftpc->count2) {
          /* try making it */
          ftpc->count2++; /* counter to prevent CWD-MKD loops */
          PPSENDF(&ftpc->pp, "MKD %s", ftpc->dirs[ftpc->count1 - 1]);
          state(conn, FTP_MKD);
        }
        else {
      else {
        /* success */
        ftpc->count2=0;
        if(++ftpc->count1 <= ftpc->dirdepth) {
          /* send next CWD */
          PPSENDF(&ftpc->pp, "CWD %s", ftpc->dirs[ftpc->count1 - 1]);
        }
        else {
          result = ftp_state_mdtm(conn);
      }
      state(conn, FTP_CWD);
      /* send CWD */
      PPSENDF(&ftpc->pp, "CWD %s", ftpc->dirs[ftpc->count1 - 1]);
      break;

    case FTP_MDTM:
