/*
    Copyright (c) 2001, Takekazu KATO All rights reserved.


    Redistribution and use in source and binary forms, with or
    without modification, are permitted provided that the following
    conditions are met:

      1. Redistributions of source code must retain the above
      copyright notice, this list of conditions and the following
      disclaimer.

      2. Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials
      provided with the distribution.</li>

     THIS SOFTWARE IS PROVIDED BY TAKEKAZU KATO ``AS IS''
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
    FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
    SHALL TAKEKAZU KATO BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
    OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
    TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
    OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
    OF SUCH DAMAGE.

     The views and conclusions contained in the software and
    documentation are those of the authors and should not be
    interpreted as representing official policies, either expressed
    or implied, of Takekazu KATO.

  $Id: endian_free.c,v 2.1.2.1 2001/12/26 05:50:30 tkato Exp $
*/
#include <stdio.h>
#include "teo.h"

/* If architecture is little endian then return 1 otherwise return 0. */
int TeoIsLittleEndian(){
  static int test=1;
  return *(char *)&test;
}

int TeoTransEndian(void *buf, int size)
{
  char *ptr;
  char b;
  int n;
  ptr = (char *)buf;
  if(size%2 != 0) return 0;
  for(n=0;n<size/2;n++){
    b = ptr[n];
    ptr[n] = ptr[size-1-n];
    ptr[size-1-n] =b;
  }
  return 1;
}

int TeoFreadEndian(void *buf, int size, int num, FILE *fp)
{
  char b;
  char *ptr;
  int n,m;
  static int test=1;
  char endian;
  endian=*(char *)&test;

  ptr = (char *)buf;
  /* little endian */
  if(endian == 1)
    for(n=0; n<num; n++){
      for(m=0; m<size; m++){
	if(fread(&b, sizeof(char), 1, fp)){
	  ptr[size - m - 1] = b;
	}else{
	  TEO_ERROR_CODE = TEO_ER_READ_ERROR;
	  return 0;
	}
      }
      ptr += size;
    }
  /* big endian */
  else 
    for(n=0; n<num; n++){
      for(m=0; m<size; m++){
	if(fread(&b, sizeof(char), 1, fp)){
	  ptr[m] = b;
	}else{
	  TEO_ERROR_CODE = TEO_ER_READ_ERROR;
	  return 0;
	}
      }
      ptr += size;
    }
  return n;
}

int TeoFwriteEndian(void *buf, int size, int num, FILE *fp)
{
  char b;
  char *ptr;
  int n,m;
  static int test=1;
  char endian;
  endian=*(char *)&test;

  ptr = (char *)buf;
  /* little endian */
  if(endian == 1)
    for(n=0; n<num; n++){
      for(m=0; m<size; m++){
	b = ptr[size - m - 1];
	if(fwrite(&b, sizeof(char),1, fp)){
	}else{
	  TEO_ERROR_CODE = TEO_ER_WRITE_ERROR;
	  return 0;
	}
      }
      ptr += size;
    }
  /* big endian */
  else
    for(n=0; n<num; n++){
      for(m=0; m<size; m++){
	b = ptr[m];
	if(fwrite(&b, sizeof(char),1, fp)){
	}else{
	  TEO_ERROR_CODE = TEO_ER_WRITE_ERROR;
	  return 0;
	}
      }
      ptr += size;
    }
  return n;
}
