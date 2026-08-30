// AUTO-DRAFT from curl/curl PR #1393
#include <base64.h>
#include <cert.h>
#include <prerror.h>
#include <keyhi.h>        /* for SECKEY_DestroyPublicKey() */
  // <<< BUG ANCHOR
#define NSSVERNUM ((NSS_VMAJOR<<16)|(NSS_VMINOR<<8)|NSS_VPATCH)

/* enough to fit the string "PEM Token #[0|1]" */
#define SLOTSIZE 13

PRFileDesc *PR_ImportTCPSocket(PRInt32 osfd);
static PRLock *nss_initlock = NULL;
static PRLock *nss_crllock = NULL;
static PRLock *nss_findslot_lock = NULL;
  PK11_SETATTRS(attrs, attr_cnt, CKA_CLASS, &obj_class, sizeof(obj_class));
  PK11_SETATTRS(attrs, attr_cnt, CKA_TOKEN, &cktrue, sizeof(CK_BBOOL));
  PK11_SETATTRS(attrs, attr_cnt, CKA_LABEL, (unsigned char *)filename,
                strlen(filename) + 1);

  if(CKO_CERTIFICATE == obj_class) {
    CK_BBOOL *pval = (cacert) ? (&cktrue) : (&ckfalse);


  /* check timeout situation */
  const long time_left = Curl_timeleft(data, NULL, TRUE);
  if(time_left < 0L) {
    failf(data, "timed out before SSL handshake");
    result = CURLE_OPERATION_TIMEDOUT;
    goto error;
