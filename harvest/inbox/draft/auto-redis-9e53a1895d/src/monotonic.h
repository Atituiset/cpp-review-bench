// AUTO-DRAFT from redis/redis PR #15710
/* Call once at startup to initialize the monotonic clock.  Though this only
 * needs to be called once, it may be called additional times without impact.
 * Returns a printable string indicating the type of clock initialized.
 * (The returned string is static and doesn't need to be freed.)  */
const char *monotonicInit(void);  // <<< BUG ANCHOR

/* Return a string indicating the type of monotonic clock being used. */
const char *monotonicInfoString(void);
