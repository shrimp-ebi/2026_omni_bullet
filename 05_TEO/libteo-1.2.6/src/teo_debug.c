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

  $Id: teo_debug.c,v 2.1.2.1 2001/12/26 05:50:30 tkato Exp $
 */
#include "define.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "teo.h"

void TeoDebugError(int,char*,int);
void TeoSetErrorMessage(int code,...);

TEOFILE *TeoDebugCreateNonGzFileWithUserExtension(char *filename,
						  int width,int height,
						  int xoffset,int yoffset,
						  int type,int bit,
						  int plane,int frame,
						  int extc,char **extv,
						  int flag,char *file,int line){
  char *p,*pp;
  int i;
  TEOFILE *retfile;
  if(filename == NULL){
    TeoSetErrorMessage(TEO_ER_NULL_FILENAME);
    TeoDebugError(flag|4,file,line);
    return NULL;
  }
  if(strcmp("-",filename) != 0){
    for(i=0,p=pp=filename;*p != '\0' && i<TEO_MAXLINE;p++,i++)
      if(*p == '.') pp = p;
    if(i >= TEO_MAXLINE){
      TeoSetErrorMessage(TEO_ER_TOO_LONG_FILENAME,filename);
      TeoDebugError(flag|4,file,line);
      return NULL;
    }
    if(strcmp(".teo",pp) != 0){
      TeoSetErrorMessage(TEO_ER_WRONG_FILE_SUFFIX,filename,"*.teo");
      TeoDebugError(flag,file,line);
    }
  }
  if((double)width*(double)height*(double)plane*(double)bit/8
     > (double)TEO_MAX_IMAGE_SIZE){
    TeoSetErrorMessage(TEO_ER_CREATE_TOO_BIG_IMAGE,
		       (double)width*(double)height*(double)plane*bit/8,
		       TEO_MAX_IMAGE_SIZE);
    TeoDebugError(flag,file,line);
  }
  retfile =  TeoCreateNonGzFileWithUserExtension(filename,
						 width, height,
						 xoffset, yoffset,
						 type, bit,
						 plane, frame,
						 extc, extv);
  if(retfile == NULL) TeoDebugError(flag|4,file,line);
  return retfile;
}

TEOFILE *TeoDebugCreateGzFileWithUserExtension(char *filename,
						int width,int height,
						int xoffset,int yoffset,
						int type,int bit,
						int plane,int frame,
						int extc,char **extv,
						int flag,char *file,int line){
  char *p,*pp;
  int i;
  TEOFILE *retfile;
  if(filename == NULL){
    TeoSetErrorMessage(TEO_ER_NULL_FILENAME);
    TeoDebugError(flag|4,file,line);
    return NULL;
  }
  if(strcmp("-",filename) != 0){
    for(i=0,p=pp=filename;*p != '\0' && i<TEO_MAXLINE;p++,i++)
      if(*p == '.') pp = p;
    if(i >= TEO_MAXLINE){
      TeoSetErrorMessage(TEO_ER_TOO_LONG_FILENAME,filename);
      TeoDebugError(flag|4,file,line);
      return NULL;
    }
    if(strcmp(".teo",pp) != 0){
      TeoSetErrorMessage(TEO_ER_WRONG_FILE_SUFFIX,filename,"*.teo");
      TeoDebugError(flag,file,line);
    }
  }
  if((double)(width)*(double)(height)*(double)(plane)*bit/8
     > (double)TEO_MAX_IMAGE_SIZE){
    TeoSetErrorMessage(TEO_ER_CREATE_TOO_BIG_IMAGE,
		       (double)(width)*(double)(height)*(double)(plane)*bit/8,
		       TEO_MAX_IMAGE_SIZE);
    TeoDebugError(flag,file,line);
  }
  retfile = TeoCreateGzFileWithUserExtension(filename,
					     width, height,
					     xoffset, yoffset,
					     type, bit,
					     plane, frame,
					     extc, extv);
  if(retfile == NULL) TeoDebugError(flag|4,file,line);
  return retfile;
}

TEOFILE *PnmDebugCreateFile(char *filename,int width,int height,
			    int type,int bit,int plane,
			    int flag,char *file,int line){
  char *p,*pp;
  int i;
  TEOFILE *retfile;
  if(filename == NULL){
    TeoSetErrorMessage(TEO_ER_NULL_FILENAME);
    TeoDebugError(flag|4,file,line);
    return NULL;
  }
  if(strcmp("-",filename) != 0){
    for(i=0,p=pp=filename;*p != '\0' && i<TEO_MAXLINE;p++,i++)
      if(*p == '.') pp = p;
    if(i >= TEO_MAXLINE){
      TeoSetErrorMessage(TEO_ER_TOO_LONG_FILENAME,filename);
      TeoDebugError(flag|4,file,line);
      return NULL;
    }
    if(strcmp(".pnm",pp) != 0 &&
       strcmp(".ppm",pp) != 0 &&
       strcmp(".pgm",pp) != 0 &&
       strcmp(".pbm",pp) != 0){
      TeoSetErrorMessage(TEO_ER_WRONG_FILE_SUFFIX,
			 filename,"*.{pnm,ppm,pgm,pbm}");
      TeoDebugError(flag,file,line);
    }
  }
  if((double)(width)*(double)(height)*(double)(plane)*bit/8
     > (double)TEO_MAX_IMAGE_SIZE){
    TeoSetErrorMessage(TEO_ER_CREATE_TOO_BIG_IMAGE,
		       (double)(width)*(double)(height)*(double)(plane)*bit/8,
		       TEO_MAX_IMAGE_SIZE);
    TeoDebugError(flag,file,line);
  }
  retfile = PnmCreateFile(filename, width, height,
			  type, bit, plane);
  if(retfile == NULL) TeoDebugError(flag|4,file,line);
  return retfile;
}

TEOFILE *PnmDebugCreateGzFile(char *filename,int width,int height,
			      int type,int bit,int plane,
			      int flag,char *file,int line){
  char *p,*pp;
  int i;
  TEOFILE *retfile;
  if(filename == NULL){
    TeoSetErrorMessage(TEO_ER_NULL_FILENAME);
    TeoDebugError(flag|4,file,line);
    return NULL;
  }
  if(strcmp("-",filename) != 0){
    for(i=0,p=pp=filename;*p != '\0' && i<TEO_MAXLINE;p++,i++)
      if(*p == '.') pp = p;
    if(i >= TEO_MAXLINE){
      TeoSetErrorMessage(TEO_ER_TOO_LONG_FILENAME,filename);
      TeoDebugError(flag|4,file,line);
      return NULL;
    }
    if(strcmp(".gz",pp) != 0){
      TeoSetErrorMessage(TEO_ER_WRONG_FILE_SUFFIX,filename,"*.gz");
      TeoDebugError(flag,file,line);
    }
  }
  if((double)(width)*(double)(height)*(double)(plane)*bit/8
     > (double)TEO_MAX_IMAGE_SIZE){
    TeoSetErrorMessage(TEO_ER_CREATE_TOO_BIG_IMAGE,
		       (double)(width)*(double)(height)*(double)(plane)*bit/8,
		       TEO_MAX_IMAGE_SIZE);
    TeoDebugError(flag,file,line);
  }
  retfile = PnmCreateGzFile(filename, width, height,
			    type, bit, plane);
  if(retfile == NULL) TeoDebugError(flag|4,file,line);
  return retfile;
}

TEOFILE *PnmDebugCreateNonGzFile(char *filename,int width,int height,
				 int type,int bit,int plane,
				 int flag,char *file,int line){
  char *p,*pp;
  int i;
  TEOFILE *retfile;
  if(filename == NULL){
    TeoSetErrorMessage(TEO_ER_NULL_FILENAME);
    TeoDebugError(flag|4,file,line);
    return NULL;
  }
  if(strcmp("-",filename) != 0){
    for(i=0,p=pp=filename;*p != '\0' && i<TEO_MAXLINE;p++,i++)
      if(*p == '.') pp = p;
    if(i >= TEO_MAXLINE){
      TeoSetErrorMessage(TEO_ER_TOO_LONG_FILENAME,filename);
      TeoDebugError(flag|4,file,line);
      return NULL;
    }
    if(strcmp(".pnm",pp) != 0 &&
       strcmp(".ppm",pp) != 0 &&
       strcmp(".pgm",pp) != 0 &&
       strcmp(".pbm",pp) != 0){
      TeoSetErrorMessage(TEO_ER_WRONG_FILE_SUFFIX,
			 filename,"*.{pnm,ppm,pgm,pbm}");
      TeoDebugError(flag,file,line);
    }
  }
  if((double)(width)*(double)(height)*(double)(plane)*bit/8
     > (double)TEO_MAX_IMAGE_SIZE){
    TeoSetErrorMessage(TEO_ER_CREATE_TOO_BIG_IMAGE,
		       (double)(width)*(double)(height)*(double)(plane)*bit/8,
		       TEO_MAX_IMAGE_SIZE);
    TeoDebugError(flag,file,line);
  }
  retfile = PnmCreateNonGzFile(filename, width, height,
			       type, bit, plane);
  if(retfile == NULL) TeoDebugError(flag|4,file,line);
  return retfile;
}

TEOFILE *TeoDebugOpenFile(char *filename,
			  int flag,char *file,int line){
  char *p,*pp;
  int i;
  TEOFILE *retfile;
  if(filename == NULL){
    TeoSetErrorMessage(TEO_ER_NULL_FILENAME);
    TeoDebugError(flag|4,file,line);
    return NULL;
  }
  if(strcmp("-",filename) != 0){
    for(i=0,p=pp=filename;*p != '\0' && i<TEO_MAXLINE;p++,i++)
      if(*p == '.') pp = p;
    if(i >= TEO_MAXLINE){
      TeoSetErrorMessage(TEO_ER_TOO_LONG_FILENAME,filename);
      TeoDebugError(flag|4,file,line);
    }
    if(strcmp(".teo",pp) != 0){
      TeoSetErrorMessage(TEO_ER_WRONG_FILE_SUFFIX,filename,"*.teo");
      TeoDebugError(flag,file,line);
    }
  }
  retfile = TeoOpenFile(filename);
  if(retfile == NULL) TeoDebugError(flag|4,file,line);
  return retfile;
}

TEOFILE *TeoDebugCreateFileWithUserExtension(char *filename,
					     int width,int height,
					     int xoffset,int yoffset,
					     int type,int bit,
					     int plane,int frame,
					     int extc,char **extv,
					     int flag,char *file,int line){
  char *p,*pp;
  int i;
  TEOFILE *retfile;
  if(filename == NULL){
    TeoSetErrorMessage(TEO_ER_NULL_FILENAME);
    TeoDebugError(flag|4,file,line);
    return NULL;
  }
  if(strcmp("-",filename) != 0){
    for(i=0,p=pp=filename;*p != '\0' && i<TEO_MAXLINE;p++,i++)
      if(*p == '.') pp = p;
    if(i >= TEO_MAXLINE){
      TeoSetErrorMessage(TEO_ER_TOO_LONG_FILENAME,filename);
      TeoDebugError(flag|4,file,line);
      return NULL;
    }
    if(strcmp(".teo",pp) != 0){
      TeoSetErrorMessage(TEO_ER_WRONG_FILE_SUFFIX,filename,"*.teo");
      TeoDebugError(flag,file,line);
    }
  }
  if((double)(width)*(double)(height)*(double)(plane)*bit/8
     > (double)TEO_MAX_IMAGE_SIZE){
    TeoSetErrorMessage(TEO_ER_CREATE_TOO_BIG_IMAGE,
		       (double)(width)*(double)(height)*(double)(plane)*bit/8,
		       TEO_MAX_IMAGE_SIZE);
    TeoDebugError(flag,file,line);
  }
  retfile = TeoCreateFileWithUserExtension(filename,
					   width, height,
					   xoffset, yoffset,
					   type, bit,
					   plane, frame,
					   extc, extv);
  if(retfile == NULL) TeoDebugError(flag|4,file,line);
  return retfile;
}


int TeoDebugCloseFileNULL(TEOFILE **teofpp,
			  int flag,char *file,int line){
  int retcode;
  if(*teofpp == NULL){
    TeoSetErrorMessage(TEO_ER_ACC_NULL_FILE);
    TeoDebugError(flag|4,file,line);
    return 0;
  }
  if(TeoCurrent(*teofpp) != TeoFrame(*teofpp)) {
    TeoSetErrorMessage(TEO_ER_CLOSE_WRONG_FRAME,
		       TeoCurrent(*teofpp),TeoFrame(*teofpp));
    TeoDebugError(flag,file,line);
  }
  retcode = TeoCloseFile(*teofpp);
  if(retcode == 0) TeoDebugError(flag|4,file,line);
  *teofpp=NULL;
  return retcode;
}

/* obsolete */
int TeoDebugCloseFile(TEOFILE *teofp,
		      int flag,char *file,int line){
  int retcode;
  if(teofp == NULL){
    TeoSetErrorMessage(TEO_ER_ACC_NULL_FILE);
    TeoDebugError(flag|4,file,line);
    return 0;
  }
  if(TeoCurrent(teofp) != TeoFrame(teofp)) {
    TeoSetErrorMessage(TEO_ER_CLOSE_WRONG_FRAME,
		       TeoCurrent(teofp),TeoFrame(teofp));
    TeoDebugError(flag,file,line);
  }
  retcode = TeoCloseFile(teofp);
  if(retcode == 0) TeoDebugError(flag|4,file,line);
  return retcode;
}

int TeoDebugReadFrame(TEOFILE *teofp,TEOIMAGE *image,
		      int flag,char *file,int line){
  int retcode;
  retcode = TeoReadFrame(teofp,image);
  if(retcode == 0) TeoDebugError(flag|4,file,line);
  return retcode;
}

int TeoDebugWriteFrame(TEOFILE *teofp,TEOIMAGE *image,
		       int flag,char *file,int line){
  int retcode;
  retcode = TeoWriteFrame(teofp,image);
  if(retcode == 0) TeoDebugError(flag|4,file,line);
  return retcode;
}

int TeoDebugSetAbsFrame(TEOFILE *teofp,int frame,
			int flag,char *file,int line){
  int retcode;
  retcode = TeoSetAbsFrame(teofp,frame);
  if(retcode == 0) TeoDebugError(flag|4,file,line);
  return retcode;
}

int TeoDebugSetRelFrame(TEOFILE *teofp,int frame,
			int flag,char *file,int line){
  int retcode;
  retcode = TeoSetRelFrame(teofp, frame);
  if(retcode == 0) TeoDebugError(flag|4,file,line);
  return retcode;
}

char *TeoDebugGetUserExtension(TEOFILE *teofp,char *keyword,
			       int flag,char *file,int line){
  char *retcode;
  retcode = TeoGetUserExtension(teofp,keyword);
  if(retcode == NULL) TeoDebugError(flag|4,file,line);
  return retcode;
}

TEOIMAGE *TeoDebugAllocImage(int width,int height,int xoffset,int yoffset,
			     int type,int bit,int plane,
			     int flag,char *file,int line){
  TEOIMAGE *retimage;
  if((double)(width)*(double)(height)*(double)(plane)*bit/8
     > (double)TEO_MAX_IMAGE_SIZE){
    TeoSetErrorMessage(TEO_ER_ALLOC_TOO_BIG_IMAGE,
		       (double)(width)*(double)(height)*(double)(plane)*bit/8,
		       TEO_MAX_IMAGE_SIZE);
    TeoDebugError(flag,file,line);
  }
  retimage = TeoAllocImage(width,height,xoffset,yoffset,type,bit,plane);
  if(retimage == NULL) TeoDebugError(flag|4,file,line);
  return retimage;
}

int TeoDebugFreeImageNULL(TEOIMAGE **imagep,int flag,char *file,int line){
  int retcode;
  retcode = TeoFreeImage(*imagep);
  if(retcode == 0) TeoDebugError(flag|4,file,line);
  *imagep = NULL;
  return retcode;
}

/* obsolete */
int TeoDebugFreeImage(TEOIMAGE *image,int flag,char *file,int line){
  int retcode;
  retcode = TeoFreeImage(image);
  if(retcode == 0) TeoDebugError(flag|4,file,line);
  return retcode;
}

void TeoDebugRangeCheck(TEOIMAGE *image,
			int index_x,
			int index_y,
			int index_p,
			size_t size,
			int flag,char *file,int line){
  if(image == NULL){
    TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE);
    TeoDebugError(flag|4,file,line);
  }
  if(size == 0){
    if(TeoBit(image) != 1){
      TeoSetErrorMessage(TEO_ER_ACC_WRONG_PSIZE,
			 1,TeoBit(image));
      TeoDebugError(flag,file,line);
    }
  } else {
    if(size*8 != TeoBit(image)){
      TeoSetErrorMessage(TEO_ER_ACC_WRONG_PSIZE,
			 size*8,TeoBit(image));
      TeoDebugError(flag,file,line);
    }
  }
  if( (index_x < TeoXstart(image)) ||
      (index_x > TeoXend(image)) ){
    TeoSetErrorMessage(TEO_ER_ACC_OUT_OF_XRANGE,index_x,
		       TeoXstart(image),
		       TeoXend(image));
    TeoDebugError(flag,file,line);
  }
  if( (index_y < TeoYstart(image)) ||
      (index_y > TeoYend(image)) ){
    TeoSetErrorMessage(TEO_ER_ACC_OUT_OF_YRANGE,index_y,
		       TeoYstart(image),
		       TeoYend(image));
    TeoDebugError(flag,file,line);
  }
  if( (index_p < 0) ||
      (index_p >= TeoPlane(image)) ){
    TeoSetErrorMessage(TEO_ER_ACC_OUT_OF_PRANGE,index_p,
		       TeoPlane(image));
    TeoDebugError(flag,file,line);
  }
}
