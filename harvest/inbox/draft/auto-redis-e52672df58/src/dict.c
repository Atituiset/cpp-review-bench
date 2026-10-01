// AUTO-DRAFT from redis/redis PR #14330
return (link) ? *link : NULL;
}

/* Find a key and return its dictEntryLink reference. Otherwise, return NULL
 * 
 * A dictEntryLink pointer being used to find preceding dictEntry of searched item.
