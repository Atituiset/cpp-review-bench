// AUTO-DRAFT from redis/redis PR #15673
c->cmd = c->realcmd = c->mstate.commands[j]->cmd;
  // <<< BUG ANCHOR
        /* ACL permissions are also checked at the time of execution in case
         * they were changed after the commands were queued. */
        int acl_errpos;
        int acl_retval = ACL_OK;
        if (!skip_acl_check) {
            acl_retval = ACLCheckAllPerm(c,&acl_errpos);
        }
        if (acl_retval != ACL_OK) {
            char *reason;
