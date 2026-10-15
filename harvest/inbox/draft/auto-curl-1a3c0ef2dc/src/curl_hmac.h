// AUTO-DRAFT from curl/curl PR #15289
#define HMAC_MD5_LENGTH 16
  // <<< BUG ANCHOR
typedef CURLcode (* HMAC_hinit_func)(void *context);
typedef void    (* HMAC_hupdate_func)(void *context,
                                      const unsigned char *data,
                                      unsigned int len);
typedef void    (* HMAC_hfinal_func)(unsigned char *result, void *context);


/* Per-hash function HMAC parameters. */
struct HMAC_params {
  HMAC_hinit_func
  hmac_hinit;     /* Initialize context procedure. */
  HMAC_hupdate_func     hmac_hupdate;   /* Update context with data. */
  HMAC_hfinal_func      hmac_hfinal;    /* Get final result procedure. */
  unsigned int          hmac_ctxtsize;  /* Context structure size. */
  unsigned int          hmac_maxkeylen; /* Maximum key length (bytes). */
  unsigned int          hmac_resultlen; /* Result length (bytes). */
};


/* HMAC computation context. */
struct HMAC_context {
  const struct HMAC_params *hmac_hash; /* Hash function definition. */
  void *hmac_hashctxt1;         /* Hash function context 1. */
  void *hmac_hashctxt2;         /* Hash function context 2. */
};
