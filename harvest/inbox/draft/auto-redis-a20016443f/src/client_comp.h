// AUTO-DRAFT from redis/redis PR #15499
int clientHasPendingCompressionFlush(struct client *c);
int clientHasPendingCompressedData(struct client *c);

/* Client-level compression API. These are the networking-facing wrappers that
 * apply the client's compressor around the client's connection.
 *
