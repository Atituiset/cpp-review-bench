// AUTO-DRAFT from postgres/postgres PR #73d4ba85059bc08b9be8f65a3fd9d1ed13eb979b
 * It's ok to cast the constness away as any modification of the none receiver
 * would be a bug (which gets easier to catch this way).
 */
DestReceiver *None_Receiver = (DestReceiver *) &donothingDR;  // <<< BUG ANCHOR

/* ----------------
 *		BeginCommand - initialize the destination at start of command
