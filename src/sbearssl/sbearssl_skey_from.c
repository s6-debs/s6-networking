/* ISC license. */

#include <bearssl.h>
#include <s6-networking/sbearssl.h>

int sbearssl_skey_from (sbearssl_skey *l, br_skey const *k, stralloc *sa)
{
  switch (k->type)
  {
    case BR_KEYTYPE_RSA :
      if (!sbearssl_rsa_skey_from(&l->data.rsa, &k->data.rsa, sa)) return 0 ;
      break ;
    case BR_KEYTYPE_EC :
      if (!sbearssl_ec_skey_from(&l->data.ec, &k->data.ec, sa)) return 0 ;
      break ;
    default :
      return 0 ;
  }
  l->type = k->type ;
  return 1 ;
}
