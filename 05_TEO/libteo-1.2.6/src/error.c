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
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "define.h"
#include "teo.h"
#include "error.h"

int TEO_ERROR_CODE;
char TEO_ERROR_MESSAGE[TEO_MAXLINE];

#ifdef TeoError
#undef TeoError
#endif
void TeoError(){
  fprintf(stderr,"[TEO ERROR:%d] %s\n",TEO_ERROR_CODE,TEO_ERROR_MESSAGE);
  exit(-1);
}
void TeoErrorBase(char *file,int line){
  fprintf(stderr,"[TEO ERROR(%s:%d)] %s\n",file,line,TEO_ERROR_MESSAGE);
  exit(-1);
}

void TeoWarn(){
  fprintf(stderr,"[TEO WARN:%d] %s\n",TEO_ERROR_CODE,TEO_ERROR_MESSAGE);
}
void TeoWarnBase(char *file,int line){
  fprintf(stderr,"[TEO WARN(%s:%d)] %s\n",file,line,TEO_ERROR_MESSAGE);
}

void TeoDebugError(int flag,char *file,int line){
  if((flag & 3) >= TEO_ERROR[TEO_ERROR_CODE].level){
    if((flag & 4) == 0) TeoWarnBase(file,line);
    else TeoErrorBase(file,line);
  }
}

void TeoSetErrorMessage(int code,...){
  va_list ap;
  TEO_ERROR_CODE = code;
  va_start(ap,code);
  vsprintf(TEO_ERROR_MESSAGE,TEO_ERROR[code].strings,ap);
  va_end(ap);
}
