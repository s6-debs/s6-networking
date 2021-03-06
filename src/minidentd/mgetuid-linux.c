/* ISC license. */

#include <sys/types.h>
#include <string.h>
#include <stdint.h>
#include <skalibs/uint64.h>
#include <skalibs/types.h>
#include <skalibs/fmtscan.h>
#include <skalibs/buffer.h>
#include <skalibs/stralloc.h>
#include <skalibs/djbunix.h>
#include <skalibs/ip46.h>
#include <skalibs/skamisc.h>
#include "mgetuid.h"

#ifdef DEBUG
#include <skalibs/strerr2.h>
#define bug(a) do { strerr_warn4x("bug parsing ", a, "remaining: ", cur) ; return 0 ; } while (0)
#else
#define bug(a) return 0
#endif

#define LINESIZE 256

static int skipspace (char **s)
{
  while (**s && ((**s == ' ') || (**s == '\t')))
    (*s)++ ;
  return (int)**s ;
}

static void reverse_address (char *s, size_t n)
{
  size_t i = n >> 1 ;
  while (i--)
  {
    char tmp = s[i] ;
    s[i] = s[n-1-i] ;
    s[n-1-i] = tmp ;
  }
}

static int parseline (char *s, size_t len, uid_t *u, char *la, uint16_t *lp, char *ra, uint16_t *rp, int is6)
{
  char *cur = s ;
  size_t pos ;
  uint64_t uu ;
  uint32_t junk ;
  unsigned int iplen = is6 ? 16 : 4 ;

  if (!skipspace(&cur)) bug("initial whitespace") ;
  pos = uint32_scan(cur, &junk) ;  /* sl */
  if (!pos || (cur-s+1+pos) > len) bug("sl") ;
  cur += pos ;
  if ((*cur++) != ':') bug("sl:") ;
  if (!skipspace(&cur)) bug("sl: SPACE") ;

  if ((cur - s + 1 + iplen) > len) bug("local_address") ;
  pos = ucharn_scan(cur, la, iplen) ;  /* local_address */
  reverse_address(la, iplen) ;
  if (!pos) bug("local_address") ;
  cur += pos ;
  if ((*cur++) != ':') bug("local_address:") ;

  pos = uint16_xscan(cur, lp) ;  /* :port */
  if (!pos || (cur-s+pos) > len) bug("local_port") ;
  cur += pos ;
  if (!skipspace(&cur)) bug("local_port SPACE") ;

  if ((cur - s + 1 + iplen) > len) bug("remote_address") ;
  pos = ucharn_scan(cur, ra, iplen) ;  /* remote_address */
  reverse_address(ra, iplen) ;
  if (!pos) bug("remote_address") ;
  cur += pos ;
  if ((*cur++) != ':') bug("remote_address:") ;

  pos = uint16_xscan(cur, rp) ;  /* :port */
  if (!pos || (cur-s+pos) > len) bug("remote_port") ;
  cur += pos ;
  if (!skipspace(&cur)) bug("remote_port SPACE");

  pos = uint32_xscan(cur, &junk) ;  /* st */
  if (!pos || (cur-s+pos) > len) bug("st") ;
  cur += pos ;
  if (!skipspace(&cur)) bug("st SPACE") ;
  pos = uint32_xscan(cur, &junk) ;  /* tx_queue */
  if (!pos || (cur-s+1+pos) > len) bug("tx_queue") ;
  cur += pos ;
  if ((*cur++) != ':') bug("tx_queue:") ;
  pos = uint32_xscan(cur, &junk) ;  /* rx_queue */
  if (!pos || (cur-s+pos) > len) bug("rx_queue") ;
  cur += pos ;
  if (!skipspace(&cur)) bug("rx_queue SPACE") ;
  pos = uint32_xscan(cur, &junk) ;  /* tr */
  if (!pos || (cur-s+1+pos) > len) bug("tr") ;
  cur += pos ;
  if ((*cur++) != ':') bug("tr:") ;
  pos = uint32_xscan(cur, &junk) ;  /* tm->when */
  if (!pos || (cur-s+pos) > len) bug("tm->when") ;
  cur += pos ;
  if (!skipspace(&cur)) bug("tm->when SPACE") ;
  pos = uint32_xscan(cur, &junk) ;  /* retrnsmt */
  if (!pos || (cur-s+pos) > len) bug("retrnsmt") ;
  cur += pos ;

  if (!skipspace(&cur)) bug("retrnsmt SPACE") ;
  pos = uint64_scan(cur, &uu) ;  /* uid */
  if (!pos || (cur-s+1+pos) > len) bug("uid") ;
  *u = uu ;
  return 1 ;
}

#ifdef DEBUG

static void debuglog (uint16_t a, uint16_t b, unsigned int c, char const *d, char const *e, int is6)
{
  char sa[UINT16_FMT] ;
  char sb[UINT16_FMT] ;
  char sc[UINT_FMT] ;
  char sd[IP46_FMT] ;
  char se[IP46_FMT] ;

  sa[uint16_fmt(sa, a)] = 0 ;
  sb[uint16_fmt(sb, b)] = 0 ;
  sc[uint_fmt(sc, c)] = 0 ;
  sd[is6 ? ip6_fmt(sd, d) : ip4_fmt(sd, d)] = 0 ;
  se[is6 ? ip6_fmt(se, e) : ip4_fmt(se, e)] = 0 ;

  buffer_puts(buffer_2, sd) ;
  buffer_puts(buffer_2, ":") ;
  buffer_puts(buffer_2, sa) ;
  buffer_puts(buffer_2, " , ") ;
  buffer_puts(buffer_2, se) ;
  buffer_puts(buffer_2, ":") ;
  buffer_puts(buffer_2, sb) ;
  buffer_puts(buffer_2, " -> ") ;
  buffer_puts(buffer_2, sc) ;
  buffer_putsflush(buffer_2, "\n") ;
}

#endif

uid_t mgetuid (ip46_t const *localaddr, uint16_t localport, ip46_t const *remoteaddr, uint16_t remoteport)
{
  int r ;
  uid_t u = -2 ;
  stralloc line = STRALLOC_ZERO ;
  buffer b ;
  char y[BUFFER_INSIZE] ;
  int is6 = ip46_is6(localaddr) ;
  int fd = open_readb(is6 ? "/proc/net/tcp6" : "/proc/net/tcp") ;
  if (fd == -1) return -2 ;
  buffer_init(&b, &buffer_read, fd, y, BUFFER_INSIZE_SMALL) ;
  if (skagetln(&b, &line, '\n') < 1) goto err ;
#ifdef DEBUG
  line.s[line.len-1] = 0 ;
  debuglog(localport, remoteport, 65535, localaddr->ip, remoteaddr->ip, is6) ;
#endif
  for (;;)
  {
    char la[16] ;
    char ra[16] ;
    uid_t nu ;
    uint16_t lp, rp ;
    line.len = 0 ;
    r = skagetln(&b, &line, '\n') ;
    if (r <= 0) { u = -1 ; break ; }
    line.s[line.len-1] = 0 ;
    if (!parseline(line.s, line.len, &nu, la, &lp, ra, &rp, is6)) break ;
#ifdef DEBUG
    debuglog(lp, rp, nu, la, ra, is6) ;
#endif
    if ((lp == localport) && (rp == remoteport)
     && !memcmp(la, localaddr->ip, is6 ? 16 : 4)
     && !memcmp(ra, remoteaddr->ip, is6 ? 16 : 4))
    {
      u = nu ; break ;
    }
  }
  stralloc_free(&line) ;
 err:
  fd_close(fd) ;
  return u ;
}
