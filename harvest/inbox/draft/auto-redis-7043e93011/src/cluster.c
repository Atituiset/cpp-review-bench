// AUTO-DRAFT from redis/redis PR #15466
* -------------------------------------------------------------------------- */
  // <<< BUG ANCHOR
/* Generates a DUMP-format representation of the object 'o', adding it to the
 * io stream pointed by 'rio'. This function can't fail. */
void createDumpPayload(rio *payload, robj *o, robj *key, int dbid, int skip_checksum) {
    unsigned char buf[2];
    uint64_t crc = 0;

    /* Serialize the object in an RDB-like format. It consist of an object type
     * byte followed by the serialized object. This is understood by RESTORE. */
    rioInitWithBuffer(payload,sdsempty());

    /* Save key metadata if present without (handles TTL separately via command args) */
    if (getModuleMetaBits(o->metabits))
        serverAssert(rdbSaveKeyMetadata(payload, key, o, dbid) != -1);
    serverAssert(rdbSaveObjectType(payload,o));
    serverAssert(rdbSaveObject(payload,o,key,dbid));

    /* If crc checksum is disabled, crc is set to 0 and no checksum validation
     * will be performed on RESTORE. */
    if (!skip_checksum) {
        /* CRC64 */
        crc = crc64(0,(unsigned char*)payload->io.buffer.ptr,
                    sdslen(payload->io.buffer.ptr));
