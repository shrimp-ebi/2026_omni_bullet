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

  $Id: define.h,v 2.1.2.1 2001/12/26 05:50:30 tkato Exp $
 */
#ifndef _DEFINE_H_
#define _DEFINE_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef DEFAULT_TMP
#define DEFAULT_TMP "/tmp"
#endif

#ifdef SUPPORT_SIGNAL_CATCH
#ifndef CATCH_SIGNALS
#define CATCH_SIGNALS {\
  SIGHUP,\
  SIGINT,\
  SIGQUIT,\
  SIGILL,\
  SIGABRT,\
  SIGFPE,\
  SIGBUS,\
  SIGSEGV,\
  SIGSYS,\
  SIGPIPE,\
  SIGTERM,\
  0}
#endif
#else
#undef CATCH_SIGNALS
#endif

#ifdef SUPPORT_GZIP
#ifndef GZIP_COMMAND
#define GZIP_COMMAND SUPPORT_GZIP " -c"
#endif
#ifndef GUNZIP_COMMAND
#define GUNZIP_COMMAND SUPPORT_GZIP " -dc"
#endif
#else
#undef GZIP_COMMAND
#undef GUNZIP_COMMAND
#undef HAVE_SYSTEM
#undef HAVE_POPEN
#endif

#endif
