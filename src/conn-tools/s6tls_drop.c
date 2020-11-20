/* ISC license. */

#include <unistd.h>
#include <stdlib.h>

#include <skalibs/strerr2.h>
#include <skalibs/types.h>

#include "s6tls-internal.h"

void s6tls_drop (void)
{
  if (!getuid())
  {
    uid_t uid ;
    gid_t gid ;
    char const *x = getenv("TLS_UID") ;
    if (x && !uid0_scan(x, &uid)) strerr_dieinvalid(100, "TLS_UID") ;
    x = getenv("TLS_GID") ;
    if (x && !gid0_scan(x, &gid)) strerr_dieinvalid(100, "TLS_GID") ;
    if (gid && setgid(gid) < 0) strerr_diefu1sys(111, "setgid") ;
    if (uid && setuid(uid) < 0) strerr_diefu1sys(111, "setuid") ;
  }
}
