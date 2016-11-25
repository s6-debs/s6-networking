/* ISC license. */

#ifndef SBEARSSL_INTERNAL_H
#define SBEARSSL_INTERNAL_H

#include <sys/types.h>
#include <bearssl.h>
#include <skalibs/stralloc.h>
#include <skalibs/genalloc.h>
#include <s6-networking/sbearssl.h>

typedef struct sbearssl_strallocerr_s sbearssl_strallocerr, *sbearssl_strallocerr_ref ;
struct sbearssl_strallocerr_s
{
  stralloc *sa ;
  int err ;
} ;

extern void sbearssl_append (void *, void const *, size_t) ;
extern int sbearssl_pem_push (br_pem_decoder_context *, char const *, size_t, sbearssl_pemobject *, genalloc *, sbearssl_strallocerr *, int *) ;

extern int sbearssl_ta_readfile_internal (char const *, genalloc *, stralloc *, genalloc *, stralloc *) ;

#endif
