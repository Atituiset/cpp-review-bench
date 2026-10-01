// AUTO-DRAFT from redis/redis PR #15623
estoreActiveExpire(db->subexpires, slot, &info);

    /* Return number of fields active-expired */
    return maxFieldsToExpire - ctx.fieldsToExpireQuota;
}
