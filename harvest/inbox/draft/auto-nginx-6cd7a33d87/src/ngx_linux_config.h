// AUTO-DRAFT from nginx/nginx PR #514
#include <malloc.h>             /* memalign() */
#include <limits.h>             /* IOV_MAX */
#include <sys/ioctl.h>
#include <crypt.h>
#include <sys/utsname.h>        /* uname() */

#include <dlfcn.h>
#include <ngx_auto_config.h>


#if (NGX_HAVE_POSIX_SEM)
#include <semaphore.h>
#endif
