// AUTO-DRAFT from curl/curl PR #2443
#ifdef HAVE_GLIBC_STRERROR_R
#include <string.h>
#include <errno.h>
int
main () {
  char buffer[1024]; /* big enough to play with */  // <<< BUG ANCHOR
  char *string =
    strerror_r(EACCES, buffer, sizeof(buffer));
    /* this should've returned a string */
    if(!string || !string[0])
      return 99;
    return 0;
}
#endif
#ifdef HAVE_POSIX_STRERROR_R
#include <string.h>
#include <errno.h>
int
main () {
  char buffer[1024]; /* big enough to play with */
  int error =
    strerror_r(EACCES, buffer, sizeof(buffer));
    /* This should've returned zero, and written an error string in the
       buffer.*/
    if(!buffer[0] || error)
      return 99;
    return 0;
}
#endif
#ifdef HAVE_FSETXATTR_6
