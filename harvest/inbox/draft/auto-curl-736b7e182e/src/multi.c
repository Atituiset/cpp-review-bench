// AUTO-DRAFT from curl/curl PR #8131
(void)nada;
}
  // <<< BUG ANCHOR

/* make sure this socket is present in the hash for this handle */
static struct Curl_sh_entry *sh_addentry(struct Curl_hash *sh,

  error:

  Curl_hash_destroy(&multi->sockhash);
  Curl_hash_destroy(&multi->hostcache);
  Curl_conncache_destroy(&multi->conn_cache);
  Curl_llist_destroy(&multi->msglist, NULL);
#if 0
/* Debug-function, used like this:
 *
 * Curl_hash_print(multi->sockhash, debug_print_sock_hash);
 *
 * Enable the hash print function first by editing hash.c
 */
static void debug_print_sock_hash(void *p)
{
  struct Curl_sh_entry *sh = (struct Curl_sh_entry *)p;

  fprintf(stderr, " [easy %p/magic %x/socket %d]",
          (void *)sh->data, sh->data->magic, (int)sh->socket);
}
#endif

  struct connectdata *conn = data->conn;
  unsigned int i;

  DEBUGF(infof(data, "multi_done"));

  if(data->state.done)
    /* Stop if multi_done() has already been called */
    /* Close all the connections in the connection cache */
    Curl_conncache_close_all_connections(&multi->conn_cache);

    Curl_hash_destroy(&multi->sockhash);
    Curl_conncache_destroy(&multi->conn_cache);
    Curl_llist_destroy(&multi->msglist, NULL);
    Curl_llist_destroy(&multi->pending, NULL);
