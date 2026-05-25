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

  $Id: fileio.c,v 2.1.2.1 2001/12/26 05:50:30 tkato Exp $
 */
#include "define.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "teo.h"

void TeoSetErrorMessage(int code,...);

#ifdef SUPPORT_SIGNAL_CATCH

#include<sys/types.h>
#include<signal.h>

static char *___teo_signal_catch_basename;
static int *___teo_signal_catch_counter;
static int ___teo_catch_signals[] = CATCH_SIGNALS;

static void ___teo_signal_catch_func(int sig){
  char file[TEO_MAXLINE];
  int i;
  for(i=0;i<*___teo_signal_catch_counter;i++){
    sprintf(file,"%s.%d.%d",___teo_signal_catch_basename,getpid(),i);
    remove(file);
  }
  signal(sig,SIG_DFL);
  kill(getpid(),sig);
}
static void ___teo_set_signal_catch(char *basename,int *cc){
  int i;

  ___teo_signal_catch_basename = basename;
  ___teo_signal_catch_counter = cc;

  for(i=0;___teo_catch_signals[i] != 0;i++)
    signal(___teo_catch_signals[i], ___teo_signal_catch_func);
}
#endif

char *TeoGenerateTmp(){
  static int counter=0;
  static char tmp_buf[TEO_MAXLINE];
  static char *pathname;
  static char basename[TEO_MAXLINE];
  char *tmpfile;
  if(counter == 0){
#ifdef SUPPORT_SIGNAL_CATCH
    ___teo_set_signal_catch(basename,&counter);
#endif
    if( (pathname = getenv("TEO_TMP_DIR") ) == NULL) pathname=DEFAULT_TMP;
    sprintf(basename,"%s/_teotmp.%ld.%d",pathname,0L,getuid());
  }
  sprintf(tmp_buf,"%s.%d.%d",basename,getpid(),counter++);
  if((tmpfile = (char *)malloc(strlen(tmp_buf)+1)) == NULL) {
    TeoSetErrorMessage(TEO_ER_CREATE_NOT_ENOUGHT_MEMORY);
    return NULL;
  }
  strcpy(tmpfile,tmp_buf);
  return tmpfile;
}

#ifdef SUPPORT_GZIP
static void TeoGzipCommand(char *src,char *dist){
  static char *gzip=NULL;
  static char command[TEO_MAXLINE];
  if(gzip == NULL){
    if((gzip = getenv("TEO_GZIP_COMMAND")) == NULL)
      gzip = GZIP_COMMAND;
  }
  if(strcmp(dist,"-") == 0)
    sprintf(command,"%s %s",gzip,src);
  else
    sprintf(command,"%s %s > %s",gzip,src,dist);

  system(command);
}

static void TeoGunzipCommand(char *src,char *dist){
  static char *gunzip=NULL;
  static char command[TEO_MAXLINE];
  if(gunzip == NULL){
    if((gunzip = getenv("TEO_GUNZIP_COMMAND")) == NULL)
      gunzip = GUNZIP_COMMAND;
  }
  if(strcmp(dist,"-") == 0)
    sprintf(command,"%s %s",gunzip,src);
  else
    sprintf(command,"%s %s > %s",gunzip,src,dist);

  system(command);
}

#ifdef HAVE_POPEN
FILE *popen(const char *command, const char *type);
static FILE *TeoGzipOpen(char *file){
  static char *gzip=NULL;
  static char command[TEO_MAXLINE];
  if(gzip == NULL){
    if((gzip = getenv("TEO_GZIP_COMMAND")) == NULL)
      gzip = GZIP_COMMAND;
  }
  if(strcmp(file,"-")==0)
    sprintf(command,"%s",gzip);
  else
    sprintf(command,"%s > %s",gzip,file);
  return popen(command,"w");
}

static FILE *TeoGunzipOpen(char *file){
  static char *gunzip=NULL;
  static char command[TEO_MAXLINE];
  if(gunzip == NULL){
    if((gunzip = getenv("TEO_GUNZIP_COMMAND")) == NULL)
      gunzip = GUNZIP_COMMAND;
  }
  if(strcmp(file,"-")==0)
    sprintf(command,"%s",gunzip);
  else
    sprintf(command,"%s %s",gunzip,file);
  return popen(command,"r");
}
#endif
#endif

TEOFILE *TeoOpenFile(char *filename){
  TEOFILE *teofp;
  int magic;
  FILE *tmpfp;
  char tmpfile2[TEO_MAXLINE];
#ifdef HAVE_POPEN
  char *use_popen;
#endif
  if(filename == NULL){
    TeoSetErrorMessage(TEO_ER_NULL_FILENAME);
    return NULL;
  }

  if((teofp=(TEOFILE *)malloc(sizeof(TEOFILE))) == NULL) {
    TeoSetErrorMessage(TEO_ER_CREATE_NOT_ENOUGHT_MEMORY);
    return NULL;
  }
  if((teofp->filename=(char *)malloc(strlen(filename)+1)) == NULL) {
    TeoSetErrorMessage(TEO_ER_CREATE_NOT_ENOUGHT_MEMORY);
    return NULL;
  }
  teofp->ac_type = TEO_AC_READ;
  teofp->extc = 0;
  teofp->extv = NULL;
  strcpy(teofp->filename,filename);

  if(strcmp(filename,"-")==0){
    teofp->fp=stdin;
    teofp->ac_type = TEO_AC_STDIN;
  } else {
    if((teofp->fp=fopen(filename,"rb")) == NULL){
      free(teofp->filename);
      free(teofp);

      TeoSetErrorMessage(TEO_ER_CANT_OPEN,filename);
      return NULL;
    }
  }

  /* preload magic number */
  magic = getc(teofp->fp);
  ungetc(magic,teofp->fp);
  
#ifdef SUPPORT_GZIP /* if gzip support */
  /* if gzip compress */
  if(magic == '\037'){
#ifdef HAVE_POPEN
    if(((use_popen=getenv("TEO_USE_POPEN"))!=NULL) &&
       (strcmp(use_popen,"yes")==0)){
      if(teofp->ac_type == TEO_AC_STDIN){
	/* if stdin */
	teofp->tmpfile = TeoGenerateTmp();
	tmpfp = fopen(teofp->tmpfile,"wb");
	while((magic = getc(teofp->fp)) != EOF) putc((char)magic,tmpfp);
	fclose(tmpfp);

	/* call gunzip */
	if( (teofp->fp = TeoGunzipOpen(teofp->tmpfile)) == NULL){
	  free(teofp->filename);
	  free(teofp->tmpfile);
	  free(teofp);
	  TeoSetErrorMessage(TEO_ER_CANT_OPEN,filename);
	  return NULL;
	}
	teofp->ac_type = TEO_AC_PIPE_SR;
      } else {
	fclose(teofp->fp);
	if( (teofp->fp = TeoGunzipOpen(teofp->filename)) == NULL){
	  free(teofp->filename);
	  free(teofp);
	  TeoSetErrorMessage(TEO_ER_CANT_OPEN,filename);
	  return NULL;
	}
	teofp->ac_type = TEO_AC_PIPE_R;
      }
    } else {
#endif
    /* generate temporary file name */

    teofp->tmpfile = TeoGenerateTmp();
    if(teofp->ac_type == TEO_AC_STDIN){
      /* if stdin */
      sprintf(tmpfile2,"%s.gz",teofp->tmpfile);
      tmpfp = fopen(tmpfile2,"wb");
      while((magic = getc(teofp->fp)) != EOF) putc((char)magic,tmpfp);
      fclose(tmpfp);

      /* call gunzip */
      TeoGunzipCommand(tmpfile2,teofp->tmpfile);
      unlink(tmpfile2);

    } else {
      fclose(teofp->fp);
      TeoGunzipCommand(filename,teofp->tmpfile);
    }

    /* open temporary file */
    teofp->ac_type = TEO_AC_TMP_R;
    if( (teofp->fp = fopen(teofp->tmpfile,"rb")) == NULL){
      free(teofp->filename);
      free(teofp->tmpfile);
      free(teofp);
      TeoSetErrorMessage(TEO_ER_CANT_OPEN,filename);
      return NULL;
    }
#ifdef HAVE_POPEN
    }
#endif
    /* preload magic number */
    magic = getc(teofp->fp);
    ungetc(magic,teofp->fp);
  }

#endif /* gzip support */

  /* if PNM format then call PnmReadHeader */
  if(magic == 'P'){
    if(PnmReadHeader(teofp) == 0){
      TeoCloseFile(teofp);
      return NULL;
    }
  } else {
    if(TeoReadHeader(teofp) == 0){
      TeoCloseFile(teofp);
      return NULL;
    }
  }
  return teofp;
}

TEOFILE *TeoCreateNonGzFileWithUserExtension(char *filename,
					     int width,int height,
					     int xoffset, int yoffset,
					     int type,int bit,
					     int plane,int frame,
					     int extc,char **extv){
  int i;
  TEOFILE *teofp;

  if(filename == NULL){
    TeoSetErrorMessage(TEO_ER_NULL_FILENAME);
    return NULL;
  }
  if(type != TEO_SIGNED && type != TEO_UNSIGNED && type != TEO_FLOAT){
    TeoSetErrorMessage(TEO_ER_CREATE_WRONG_TYPE,type);
    return NULL;
  }

  if(bit != 1 && bit != 8 &&
     bit != 16 && bit != 32 &&
     (!( (type == TEO_FLOAT && bit==64) ))){
    TeoSetErrorMessage(TEO_ER_CREATE_WRONG_PSIZE,type);
    return NULL;
  }

  if(width <= 0 || height <= 0 || plane <= 0 || frame <= 0){
    TeoSetErrorMessage(TEO_ER_CREATE_WRONG_SIZE,width,height,plane,frame);
    return NULL;
  }

  if((teofp=(TEOFILE *)malloc(sizeof(TEOFILE))) == NULL) {
    TeoSetErrorMessage(TEO_ER_CREATE_NOT_ENOUGHT_MEMORY);
    return NULL;
  }
  if((teofp->filename=(char *)malloc(strlen(filename)+1)) == NULL) {
    TeoSetErrorMessage(TEO_ER_CREATE_NOT_ENOUGHT_MEMORY);
    return NULL;
  }

  teofp->width = width;
  teofp->height = height;
  teofp->xoffset = xoffset;
  teofp->yoffset = yoffset;
  teofp->type = type;
  teofp->bit = bit;
  teofp->plane = plane;
  teofp->frame = frame;
 
  if(extc > 0){
    teofp->extc = extc;
    if((teofp->extv = (char **)malloc(sizeof(char *)*extc)) == NULL) {
      TeoSetErrorMessage(TEO_ER_CREATE_NOT_ENOUGHT_MEMORY);
      return NULL;
    }
    for(i=0;i<extc;i++){
      if((teofp->extv[i] = (char *)malloc(sizeof(char)*strlen(extv[i])+1))
	 == NULL) {
	TeoSetErrorMessage(TEO_ER_CREATE_NOT_ENOUGHT_MEMORY);
	return NULL;
      }
      strcpy(teofp->extv[i],extv[i]);
    }
  } else {
    teofp->extc = 0;
    teofp->extv = NULL;
  }

  strcpy(teofp->filename,filename);


  if(strcmp(filename,"-")==0){
    teofp->fp=stdout;
    teofp->ac_type = TEO_AC_STDOUT;
  } else {
    if((teofp->fp=fopen(filename,"wb")) == NULL){
      free(teofp->filename);
      free(teofp);
      TeoSetErrorMessage(TEO_ER_CANT_OPEN,filename);
      return NULL;
    }
    teofp->ac_type = TEO_AC_WRITE;
  }

  if(TeoWriteHeader(teofp) == 0){
    TeoCloseFile(teofp);
    return 0;
  }
  teofp->current = 0;
  return teofp;
}

TEOFILE *TeoCreateGzFileWithUserExtension(char *filename,
					  int width,int height,
					  int xoffset, int yoffset,
					  int type,int bit,
					  int plane,int frame,
					  int extc,char **extv){
  int i;
  TEOFILE *teofp;
#ifdef HAVE_POPEN
  char *use_popen;
#endif

  if(filename == NULL){
    TeoSetErrorMessage(TEO_ER_NULL_FILENAME);
    return NULL;
  }
  if(type != TEO_SIGNED && type != TEO_UNSIGNED && type != TEO_FLOAT){
    TeoSetErrorMessage(TEO_ER_CREATE_WRONG_TYPE,type);
    return NULL;
  }

  if(bit != 1 && bit != 8 &&
     bit != 16 && bit != 32 &&
     (!( (type == TEO_FLOAT && bit==64) ))){
    TeoSetErrorMessage(TEO_ER_CREATE_WRONG_PSIZE,type);
    return NULL;
  }

  if(width <= 0 || height <= 0 || plane <= 0 || frame <= 0){
    TeoSetErrorMessage(TEO_ER_CREATE_WRONG_SIZE,width,height,plane,frame);
    return NULL;
  }

  if((teofp=(TEOFILE *)malloc(sizeof(TEOFILE))) == NULL) {
    TeoSetErrorMessage(TEO_ER_CREATE_NOT_ENOUGHT_MEMORY);
    return NULL;
  }
  if((teofp->filename=(char *)malloc(strlen(filename)+1)) == NULL) {
    TeoSetErrorMessage(TEO_ER_CREATE_NOT_ENOUGHT_MEMORY);
    return NULL;
  }

  teofp->width = width;
  teofp->height = height;
  teofp->xoffset = xoffset;
  teofp->yoffset = yoffset;
  teofp->type = type;
  teofp->bit = bit;
  teofp->plane = plane;
  teofp->frame = frame;

  if(extc > 0){
    teofp->extc = extc;
    if((teofp->extv = (char **)malloc(sizeof(char *)*extc))
       == NULL) {
      TeoSetErrorMessage(TEO_ER_CREATE_NOT_ENOUGHT_MEMORY);
      return NULL;
    }
    for(i=0;i<extc;i++){
      if((teofp->extv[i] = (char *)malloc(sizeof(char)*strlen(extv[i])+1))
	 == NULL) {
	TeoSetErrorMessage(TEO_ER_CREATE_NOT_ENOUGHT_MEMORY);
	return NULL;
      }
      strcpy(teofp->extv[i],extv[i]);
    }
  } else {
    teofp->extc = 0;
    teofp->extv = NULL;
  }

  strcpy(teofp->filename,filename);


#ifdef SUPPORT_GZIP /* gzip support */
#ifdef HAVE_POPEN
  if(((use_popen=getenv("TEO_USE_POPEN"))!=NULL) &&
      (strcmp(use_popen,"yes")==0)){
    teofp->ac_type = TEO_AC_PIPE_W;
    if((teofp->fp=TeoGzipOpen(teofp->filename)) == NULL){
      free(teofp->filename);
      free(teofp);
      TeoSetErrorMessage(TEO_ER_CANT_CREATE,filename);
      return NULL;
    }
  } else {
#endif
    if(strcmp(filename,"-") == 0)
      teofp->ac_type = TEO_AC_TMP_SW;
    else 
      teofp->ac_type = TEO_AC_TMP_W;

    teofp->tmpfile = TeoGenerateTmp();
    if((teofp->fp=fopen(teofp->tmpfile,"wb")) == NULL){
      free(teofp->filename);
      free(teofp->tmpfile);
      free(teofp);
      TeoSetErrorMessage(TEO_ER_CANT_CREATE,filename);
      return NULL;
    }
#ifdef HAVE_POPEN
  }
#endif
#else /* gzip not support */
  if(strcmp(filename,"-") == 0){
    teofp->fp=stdout;
    teofp->ac_type = TEO_AC_STDOUT;
  } else {
    if((teofp->fp=fopen(filename,"wb")) == NULL){
      free(teofp->filename);
      free(teofp);
      TeoSetErrorMessage(TEO_ER_CANT_CREATE,filename);
      return NULL;
    }
    teofp->ac_type = TEO_AC_WRITE;
  }
#endif

  if(TeoWriteHeader(teofp) == 0){
    TeoCloseFile(teofp);
    return 0;
  }
  teofp->current = 0;
  return teofp;
}

TEOFILE *PnmCreateNonGzFile(char *filename,int width,int height,
			    int type,int bit,int plane){
  TEOFILE *teofp;

  if(filename == NULL){
    TeoSetErrorMessage(TEO_ER_NULL_FILENAME);
    return NULL;
  }
  if(type != TEO_SIGNED && type != TEO_UNSIGNED && type != TEO_FLOAT){
    TeoSetErrorMessage(TEO_ER_CREATE_WRONG_TYPE,type);
    return NULL;
  }

  if(bit != 1 && bit != 8 &&
     bit != 16 && bit != 32 &&
     (!( (type == TEO_FLOAT && bit==64) ))){
    TeoSetErrorMessage(TEO_ER_CREATE_WRONG_PSIZE,type);
    return NULL;
  }

  if(width <= 0 || height <= 0 || plane <= 0){
    TeoSetErrorMessage(TEO_ER_CREATE_WRONG_SIZE,width,height,plane,0);
    return NULL;
  }

  if((teofp=(TEOFILE *)malloc(sizeof(TEOFILE))) == NULL) {
    TeoSetErrorMessage(TEO_ER_CREATE_NOT_ENOUGHT_MEMORY);
    return NULL;
  }
  if((teofp->filename=(char *)malloc(strlen(filename)+1)) == NULL) {
    TeoSetErrorMessage(TEO_ER_CREATE_NOT_ENOUGHT_MEMORY);
    return NULL;
  }

  teofp->width = width;
  teofp->height = height;
  teofp->xoffset = 0;
  teofp->yoffset = 0;
  teofp->type = type;
  teofp->bit = bit;
  teofp->plane = plane;
  teofp->frame = 1;
  teofp->extc = 0;
  teofp->extv = NULL;
  strcpy(teofp->filename,filename);

  if(strcmp(filename,"-")==0){
    teofp->fp=stdout;
    teofp->ac_type = TEO_AC_STDOUT;
  } else {
    if((teofp->fp=fopen(filename,"wb")) == NULL){
      free(teofp->filename);
      free(teofp);
      TeoSetErrorMessage(TEO_ER_CANT_CREATE,filename);
      return NULL;
    }
    teofp->ac_type = TEO_AC_WRITE;
  }
  if(PnmWriteHeader(teofp) == 0){
    TeoCloseFile(teofp);
    return NULL;
  }
  teofp->current = 0;
  return teofp;
}



TEOFILE *PnmCreateGzFile(char *filename,int width,int height,
		   int type,int bit,int plane){
  TEOFILE *teofp;
#ifdef HAVE_POPEN
  char *use_popen;
#endif

  if(filename == NULL){
    TeoSetErrorMessage(TEO_ER_NULL_FILENAME);
    return NULL;
  }
  if(type != TEO_SIGNED && type != TEO_UNSIGNED && type != TEO_FLOAT){
    TeoSetErrorMessage(TEO_ER_CREATE_WRONG_TYPE,type);
    return NULL;
  }

  if(bit != 1 && bit != 8 &&
     bit != 16 && bit != 32 &&
     (!( (type == TEO_FLOAT && bit==64) ))){
    TeoSetErrorMessage(TEO_ER_CREATE_WRONG_PSIZE,type);
    return NULL;
  }

  if(width <= 0 || height <= 0 || plane <= 0){
    TeoSetErrorMessage(TEO_ER_CREATE_WRONG_SIZE,width,height,plane,0);
    return NULL;
  }

  if((teofp=(TEOFILE *)malloc(sizeof(TEOFILE))) == NULL) {
    TeoSetErrorMessage(TEO_ER_CREATE_NOT_ENOUGHT_MEMORY);
    return NULL;
  }
  if((teofp->filename=(char *)malloc(strlen(filename)+1)) == NULL) {
    TeoSetErrorMessage(TEO_ER_CREATE_NOT_ENOUGHT_MEMORY);
    return NULL;
  }

  teofp->width = width;
  teofp->height = height;
  teofp->xoffset = 0;
  teofp->yoffset = 0;
  teofp->type = type;
  teofp->bit = bit;
  teofp->plane = plane;
  teofp->frame = 1;
  teofp->extc = 0;
  teofp->extv = NULL;

  strcpy(teofp->filename,filename);


#ifdef SUPPORT_GZIP
#ifdef HAVE_POPEN
  if(((use_popen=getenv("TEO_USE_POPEN"))!=NULL) &&
     (strcmp(use_popen,"yes")==0)){
    teofp->ac_type = TEO_AC_PIPE_W;
    if((teofp->fp=TeoGzipOpen(teofp->filename)) == NULL){
      free(teofp->filename);
      free(teofp);
      TeoSetErrorMessage(TEO_ER_CANT_CREATE,filename);
      return NULL;
    }
  } else {
#endif
  if(strcmp(filename,"-")==0)
    teofp->ac_type = TEO_AC_TMP_SW;
  else 
    teofp->ac_type = TEO_AC_TMP_W;

  teofp->tmpfile = TeoGenerateTmp();
  if((teofp->fp=fopen(teofp->tmpfile,"wb")) == NULL){
    free(teofp->filename);
    free(teofp->tmpfile);
    free(teofp);
    TeoSetErrorMessage(TEO_ER_CANT_CREATE,filename);
    return NULL;
  }
#ifdef HAVE_POPEN
  }
#endif
#else  /* gzip not support */
  if(strcmp(filename,"-")==0){
    teofp->fp=stdout;
  } else {
    if((teofp->fp=fopen(filename,"wb")) == NULL){
      free(teofp->filename);
      free(teofp);
      TeoSetErrorMessage(TEO_ER_CANT_CREATE,filename);
      return NULL;
    }
  }
#endif

  if(PnmWriteHeader(teofp) == 0){
    TeoCloseFile(teofp);
    return NULL;
  }
  teofp->current = 0;
  return teofp;
}


TEOFILE *TeoCreateFileWithUserExtension(char *filename,
					int width,int height,
					int xoffset,int yoffset,
					int type,int bit,
					int plane,int frame,
					int extc,char **extv){
#ifdef SUPPORT_GZIP
  char *tmp;
  if((tmp = getenv("TEO_GZIP")) == NULL)
    return TeoCreateNonGzFileWithUserExtension(filename,width,height,
					       xoffset,yoffset,
					       type,bit,plane,frame,
					       extc,extv);
  else if(strcmp(tmp,"yes") == 0)
    return TeoCreateGzFileWithUserExtension(filename,width,height,
					    xoffset,yoffset,
					    type,bit,plane,frame,
					    extc,extv);
  else 
    return TeoCreateNonGzFileWithUserExtension(filename,width,height,
					       xoffset,yoffset,
					       type,bit,plane,frame,
					       extc,extv);
#else
  return TeoCreateNonGzFileWithUserExtension(filename,width,height,
					     xoffset,yoffset,
					     type,bit,plane,frame,
					     extc,extv);
#endif
}

TEOFILE *PnmCreateFile(char *filename,int width,int height,
		     int type,int bit,int plane){
#ifdef SUPPORT_GZIP
  char *tmp;
  if((tmp = getenv("TEO_GZIP")) == NULL)
    return PnmCreateNonGzFile(filename,width,height,type,bit,plane);
  if(strcmp(tmp,"yes") == 0)
    return PnmCreateGzFile(filename,width,height,type,bit,plane);
  else 
    return PnmCreateNonGzFile(filename,width,height,type,bit,plane);
#else
  return PnmCreateNonGzFile(filename,width,height,type,bit,plane);
#endif
}

int TeoCloseFile(TEOFILE *teofp){
  int i;
  if(teofp == NULL){
    TeoSetErrorMessage(TEO_ER_ACC_NULL_FILE);
    return 0;
  }
  if(teofp->ac_type == TEO_AC_READ ||
     teofp->ac_type == TEO_AC_WRITE){
    fclose(teofp->fp);
#ifdef SUPPORT_GZIP
  } else if(teofp->ac_type == TEO_AC_TMP_R){
    fclose(teofp->fp);
    unlink(teofp->tmpfile);
    free(teofp->tmpfile);
  } else if(teofp->ac_type == TEO_AC_TMP_W){
    fclose(teofp->fp);

    TeoGzipCommand(teofp->tmpfile,teofp->filename);
    unlink(teofp->tmpfile);
    free(teofp->tmpfile);
  } else if(teofp->ac_type == TEO_AC_TMP_SW){
    fclose(teofp->fp);
    TeoGzipCommand(teofp->tmpfile,teofp->filename);
    unlink(teofp->tmpfile);
    free(teofp->tmpfile);
#ifdef HAVE_POPEN
  } else if(teofp->ac_type == TEO_AC_PIPE_SR){
    pclose(teofp->fp);
    unlink(teofp->tmpfile);
    free(teofp->tmpfile);
  } else if(teofp->ac_type == TEO_AC_PIPE_R ||
	    teofp->ac_type == TEO_AC_PIPE_W){
    pclose(teofp->fp);
#endif
#endif
  }
  if(teofp->extc > 0){
    for(i=0;i<teofp->extc;i++)
      free(teofp->extv[i]);
    free(teofp->extv);
  }
  free(teofp->filename);

  teofp->width = teofp->height =
    teofp->xoffset = teofp->yoffset = 
    teofp->type = teofp->bit =
    teofp->plane = teofp->frame = 
    teofp->current = teofp->fsize =
    teofp->hsize = 0;
  teofp->fp = NULL;
  teofp->filename = NULL;
  teofp->tmpfile = NULL;

  teofp->ac_type = 0;

  free(teofp);
  return 1;
}

int TeoReadHeader(TEOFILE *teofp){
  char line[TEO_MAXLINE];
  char *stringp;
  char tmp[TEO_MAXLINE];
  char magic[TEO_MAXLINE];
  int version;
  int snum;
  int i;
  char *extmp[TEO_MAXLINE];

  teofp->extc = 0;
  teofp->extv = NULL;

  /* check magic number and version number */
  if(fgets(line,TEO_MAXLINE,teofp->fp)==NULL) {
    TeoSetErrorMessage(TEO_ER_READ_CANT_GET_FORMAT,teofp->filename);
    return 0;
  }

  if(sscanf(line,"%s%d",magic,&version) != 2){
    TeoSetErrorMessage(TEO_ER_READ_CANT_GET_FORMAT,teofp->filename);
    return 0;
  }

  /* check magic */
  if(memcmp(magic,"TEO",3) != 0) {
    TeoSetErrorMessage(TEO_ER_READ_WRONG_MAGIC,teofp->filename);
    return 0;
  }
  /* check version */
  if(version > TEO_VERSION ) {
    TeoSetErrorMessage(TEO_ER_READ_WRONG_VERSION,teofp->filename);
    return 0;
  }


  /* get image size */
  do {
    if(fgets(line,TEO_MAXLINE,teofp->fp)==NULL){
      TeoSetErrorMessage(TEO_ER_READ_CANT_GET_SIZE,teofp->filename);
      return 0;
    }
    if((line)[0] == '#' && (line)[1] == '%'){
      stringp = (line + 2);
      while(*stringp == '\040' || *stringp == '\010') stringp++;
      stringp[strlen(stringp)-1] = '\0';
      if((extmp[teofp->extc] = (char *)malloc(strlen(stringp)+1)) == NULL) {
	TeoSetErrorMessage(TEO_ER_CREATE_NOT_ENOUGHT_MEMORY);
	return 0;
      }
      strcpy(extmp[teofp->extc],stringp);
      teofp->extc++;
    }
  } while((line)[0] == '#');


  if((snum = sscanf(line,"%d%d%d%d",
		    &teofp->width,&teofp->height,
		    &teofp->xoffset,&teofp->yoffset)) < 2){
    TeoSetErrorMessage(TEO_ER_READ_CANT_GET_SIZE,teofp->filename);
    return  0;
  } else if (snum < 4){
    teofp->xoffset = 0;
    teofp->yoffset = 0;
  }

  /* get pixel type and size */
  do{
    if(fgets(line,TEO_MAXLINE,teofp->fp)==NULL) {
      TeoSetErrorMessage(TEO_ER_READ_CANT_GET_PTYPE,teofp->filename);
      return 0;
    }
    if((line)[0] == '#' && (line)[1] == '%'){
      stringp = (line + 2);
      while(*stringp == '\040' || *stringp == '\010') stringp++;
      stringp[strlen(stringp)-1] = '\0';
      if((extmp[teofp->extc] = (char *)malloc(strlen(stringp)+1)) == NULL) {
	TeoSetErrorMessage(TEO_ER_CREATE_NOT_ENOUGHT_MEMORY);
	return 0;
      }
      strcpy(extmp[teofp->extc],stringp);
      teofp->extc++;
    }
  } while((line)[0] == '#');

  if(sscanf(line,"%s%d",tmp,&teofp->bit) != 2){
    TeoSetErrorMessage(TEO_ER_READ_CANT_GET_PTYPE,teofp->filename);
    return 0;
  }

  switch(tmp[0]){
  case 'S':
    teofp->type=TEO_SIGNED;
    break;
  case 'U':
    teofp->type=TEO_UNSIGNED;
    break;
  case 'F':
    teofp->type=TEO_FLOAT;
    break;
  default:
    TeoSetErrorMessage(TEO_ER_READ_WRONG_PTYPE,teofp->filename);
    return 0;
    break;
  }

  if(teofp->bit != 1 && teofp->bit != 8 &&
     teofp->bit != 16 && teofp->bit != 32 &&
     (!( (teofp->type == TEO_FLOAT && teofp->bit==64) ))){
    TeoSetErrorMessage(TEO_ER_READ_WRONG_PSIZE,teofp->filename);
    return 0;
  }
  /* get number of planes and frames */
  do{
    if(fgets(line,TEO_MAXLINE,teofp->fp)==NULL){
      TeoSetErrorMessage(TEO_ER_READ_CANT_GET_PLANE,teofp->filename);
      return 0;
    }
    if((line)[0] == '#' && (line)[1] == '%'){
      stringp = (line + 2);
      while(*stringp == '\040' || *stringp == '\010') stringp++;
      stringp[strlen(stringp)-1] = '\0';
      if((extmp[teofp->extc] = (char *)malloc(strlen(stringp)+1)) == NULL) {
	TeoSetErrorMessage(TEO_ER_CREATE_NOT_ENOUGHT_MEMORY);
	return 0;
      }
      strcpy(extmp[teofp->extc],stringp);
      teofp->extc++;
    }
  } while((line)[0] == '#');

  if(sscanf(line,"%d%d",&teofp->plane,&teofp->frame) != 2){
    TeoSetErrorMessage(TEO_ER_READ_CANT_GET_PLANE,teofp->filename);
    return 0;
  }

  if(teofp->extc>0){
    if((teofp->extv = (char **)malloc(sizeof(char *)*teofp->extc)) == NULL) {
      TeoSetErrorMessage(TEO_ER_CREATE_NOT_ENOUGHT_MEMORY);
      return 0;
    }
    for(i=0;i<teofp->extc;i++)
      teofp->extv[i] = extmp[i];
  }

  /* calculate frame size */
  teofp->fsize = (teofp->bit == 1)? 
    (((int)((teofp->width-1)/8) + 1) * teofp->height * teofp->plane) : 
      (teofp->width * teofp->height * (teofp->bit/8) * teofp->plane);
  
  teofp->hsize = ftell(teofp->fp);

  if(teofp->fsize <= 0) {
    TeoSetErrorMessage(TEO_ER_READ_WRONG_FSIZE,teofp->filename);
    return 0;
  }
  teofp->current = 0;
  return teofp->fsize;
}


int TeoWriteHeader(TEOFILE *teofp){
  int i;
  /* magic, version, and library version  */
  fprintf(teofp->fp,"TEO %d # generated by %s.\012",
	  TEO_VERSION,TEO_LIB_VERSION);

  /* user extention value */
  for(i=0;i<teofp->extc;i++)
    fprintf(teofp->fp,"#%% %s\012",teofp->extv[i]);

  /* image size */
  fprintf(teofp->fp,"%d %d %d %d\012",
	  teofp->width,teofp->height,
	  teofp->xoffset,teofp->yoffset);
  
  if(teofp->bit != 1 && teofp->bit != 8 &&
     teofp->bit != 16 && teofp->bit != 32 && 
     (!( (teofp->type == TEO_FLOAT && teofp->bit==64) ))){
    TeoSetErrorMessage(TEO_ER_WRITE_WRONG_PSIZE,teofp->filename);
    return 0;
  }
  /* pixel type and size */
  switch(teofp->type){
  case TEO_SIGNED:
    fprintf(teofp->fp,"S %d\012",teofp->bit);
    break;
  case TEO_UNSIGNED:
    fprintf(teofp->fp,"U %d\012",teofp->bit);
    break;
  case TEO_FLOAT:
    fprintf(teofp->fp,"F %d\012",teofp->bit);
    break;
  default:
    TeoSetErrorMessage(TEO_ER_WRITE_WRONG_PTYPE,teofp->filename);
    return 0;
  }

  /* number of planes and frames */
  fprintf(teofp->fp,"%d %d\012",teofp->plane,teofp->frame);

  teofp->fsize = (teofp->bit == 1)? 
    (((int)((teofp->width-1)/8) + 1) * teofp->height * teofp->plane) : 
      (teofp->width * teofp->height * (teofp->bit/8) * teofp->plane);
  
  teofp->hsize = ftell(teofp->fp);
  
  if(teofp->fsize <= 0) {
    TeoSetErrorMessage(TEO_ER_WRITE_WRONG_FSIZE,teofp->filename);
    return 0;
  }
  teofp->current = 0;
  return teofp->fsize;
}

// read a non-negative integer from the source.
// return -1 upon error

static int read_number(FILE *fp) {
  int number;
  unsigned char ch;
  
  // Initialize return value
  number = 0;
  // Skip leading whitespace
  do {
    if ( ! fread(&ch, 1, 1, fp) ) {
      return -1;
    }
    // Eat comments as whitespace
    if ( ch == '#' ) {  // Comment is '#' to end of line
      do {
	if ( ! fread(&ch, 1, 1, fp) ) {
	  return -1;
	}
      } while ( (ch != '\r') && (ch != '\n') );
    }
  } while ( isspace(ch) );
  // Add up the number
  do {
    number *= 10;
    number += ch-'0';
    if ( ! fread(&ch, 1, 1, fp) ) {
      return -1;
    }
  } while ( isdigit(ch) );
  return(number);
}

int PnmReadHeader(TEOFILE *teofp){
  char line[TEO_MAXLINE];
  char magic[TEO_MAXLINE];

  teofp->extc = 0;
  teofp->extv = NULL;
  teofp->xoffset = 0;
  teofp->yoffset = 0;

  if(fgets(line,TEO_MAXLINE,teofp->fp)==NULL){
    TeoSetErrorMessage(TEO_ER_READ_CANT_GET_FORMAT,teofp->filename);
    return 0;
  }

  if(sscanf(line,"%s",magic) != 1){
    TeoSetErrorMessage(TEO_ER_READ_CANT_GET_FORMAT,teofp->filename);
    return 0;
  }

  /* check magic */
  if(memcmp(magic,"P4",2) == 0){
    /* if PBM(raw) format */
    if( (teofp->width = read_number(teofp->fp)) < 0) {
      TeoSetErrorMessage(TEO_ER_READ_CANT_GET_SIZE,teofp->filename);
      return  0;
    }
    if( (teofp->height = read_number(teofp->fp)) < 0) {
      TeoSetErrorMessage(TEO_ER_READ_CANT_GET_SIZE,teofp->filename);
      return  0;
    }
    teofp->type = TEO_UNSIGNED;
    teofp->bit = 1;
    teofp->plane = 1;
    teofp->frame = 1;
  } else if(memcmp(magic,"P5",2) == 0){
    int graylevel;

    /* if PGM(raw) */
    if( (teofp->width = read_number(teofp->fp)) < 0) {
      TeoSetErrorMessage(TEO_ER_READ_CANT_GET_SIZE,teofp->filename);
      return  0;
    }
    if( (teofp->height = read_number(teofp->fp)) < 0) {
      TeoSetErrorMessage(TEO_ER_READ_CANT_GET_SIZE,teofp->filename);
      return  0;
    }
    if( (graylevel  = read_number(teofp->fp)) < 0) {
      TeoSetErrorMessage(TEO_ER_READ_CANT_GET_SIZE,teofp->filename);
      return  0;
    }
    if(graylevel < 256){
      teofp->bit = 8;
    } else if(graylevel < 65536){
      teofp->bit = 16;
    } else {
      teofp->bit = 32;
    }
    teofp->type = TEO_UNSIGNED;
    teofp->plane = 1;
    teofp->frame = 1;
  } else if(memcmp(magic,"P6",2) == 0){
    int graylevel;

    /* if PPM(raw) format */
    if( (teofp->width = read_number(teofp->fp)) < 0) {
      TeoSetErrorMessage(TEO_ER_READ_CANT_GET_SIZE,teofp->filename);
      return  0;
    }
    if( (teofp->height = read_number(teofp->fp)) < 0) {
      TeoSetErrorMessage(TEO_ER_READ_CANT_GET_SIZE,teofp->filename);
      return  0;
    }
    if( (graylevel  = read_number(teofp->fp)) < 0) {
      TeoSetErrorMessage(TEO_ER_READ_CANT_GET_SIZE,teofp->filename);
      return  0;
    }
    if(graylevel < 256){
      teofp->bit = 8;
    } else if(graylevel < 65536){
      teofp->bit = 16;
    } else {
      teofp->bit = 32;
    }
    teofp->type = TEO_UNSIGNED;
    teofp->plane = 3;
    teofp->frame = 1;
  } else {
    TeoSetErrorMessage(TEO_ER_READ_WRONG_MAGIC,teofp->filename);
    return 0;
  }

  /* calculate frame size */
  teofp->fsize = (teofp->bit == 1)? 
    (((int)((teofp->width-1)/8) + 1) * teofp->height * teofp->plane) : 
      (teofp->width * teofp->height * (teofp->bit/8) * teofp->plane);
  
  teofp->hsize = ftell(teofp->fp);

  if(teofp->fsize <= 0){
    TeoSetErrorMessage(TEO_ER_READ_WRONG_FSIZE,teofp->filename);
    return 0;
  }
  teofp->current = 0;
  return teofp->fsize;
}

int PnmWriteHeader(TEOFILE *teofp){
  char magic[3];

  if(teofp->frame != 1){
    TeoSetErrorMessage(TEO_ER_WRITE_WRONG_MAGIC,teofp->filename);
    return 0;
  }

  /* set magic */
  if(teofp->bit == 1 &&
     teofp->plane == 1)
    strcpy(magic,"P4");
  else if(teofp->type == TEO_UNSIGNED &&
	    teofp->plane == 1)
    strcpy(magic,"P5");
  else if(teofp->type == TEO_UNSIGNED &&
	  teofp->plane == 3)
    strcpy(magic,"P6");
  else {
    TeoSetErrorMessage(TEO_ER_WRITE_WRONG_PTYPE,teofp->filename);
    return 0;
  }

  fprintf(teofp->fp,"%s # generated by %s.\012",
	    magic,TEO_LIB_VERSION);

  /* image size */
  fprintf(teofp->fp,"%d %d\012",teofp->width,teofp->height);
  if(strcmp(magic,"P4")){
    if(teofp->bit == 8) 
      fprintf(teofp->fp,"255\012");
    else if(teofp->bit == 16)
      fprintf(teofp->fp,"65535\012");
    else if(teofp->bit == 32)
      fprintf(teofp->fp,"4294967295\01b2");
    else {
      TeoSetErrorMessage(TEO_ER_WRITE_WRONG_PTYPE,teofp->filename);
      return 0;
    }
  }
  teofp->fsize = (teofp->bit == 1)? 
    (((int)((teofp->width-1)/8) + 1) * teofp->height * teofp->plane) : 
    (teofp->width * teofp->height * (teofp->bit/8) * teofp->plane);
  
  teofp->hsize = ftell(teofp->fp);

  if(teofp->fsize <= 0 ){
    TeoSetErrorMessage(TEO_ER_WRITE_WRONG_FSIZE,teofp->filename);
    return 0;
  }
  teofp->current = 0;
  return teofp->fsize;
}

int TeoReadFrame(TEOFILE *teofp,TEOIMAGE *image){
  if(teofp == NULL){
    TeoSetErrorMessage(TEO_ER_ACC_NULL_FILE);
    return 0;
  }
  if(image == NULL){
    TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE);
    return 0;
  }
  if(teofp->frame > 0 && teofp->current >= teofp->frame){
    TeoSetErrorMessage(TEO_ER_OUT_OF_FRAME,
		       teofp->filename,
		       teofp->current,
		       teofp->frame);
    return 0;
  }

  if( TeoBit(teofp) != TeoBit(image) ){
    TeoSetErrorMessage(TEO_ER_CANT_READ_FRAME_DIFF_BIT,
		       TeoBit(teofp),TeoBit(image));
    return 0;
  }
  if( (TeoWidth(teofp) != TeoWidth(image)) ||
      (TeoHeight(teofp) != TeoHeight(image)) ||
      (TeoPlane(teofp) != TeoPlane(image)) ){
    TeoSetErrorMessage(TEO_ER_CANT_READ_FRAME_DIFF_SIZE,
		       TeoWidth(teofp),TeoHeight(teofp),TeoPlane(teofp),
		       TeoWidth(image),TeoHeight(image),TeoPlane(image));
    return 0;
  }
		       

  teofp->current++;

  if(teofp->bit <= 8){
    if(TeoFreadEndian((char *)(image)->data,1,
		      (teofp)->fsize,(teofp)->fp) == 0){
      TeoSetErrorMessage(TEO_ER_CANT_READ_FRAME,teofp->filename);
      return 0;
    }
  } else {
    if(TeoFreadEndian((char *)(image)->data,(int)((teofp)->bit/8),
		      (teofp)->width * (teofp)->height * (teofp)->plane,
		      (teofp)->fp) == 0){
      TeoSetErrorMessage(TEO_ER_CANT_READ_FRAME,teofp->filename);
      return 0;
    }
  }
  return teofp->current;
}

int TeoWriteFrame(TEOFILE *teofp,TEOIMAGE *image){
  if(teofp == NULL){
    TeoSetErrorMessage(TEO_ER_ACC_NULL_FILE);
    return 0;
  }
  if(image == NULL){
    TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE);
    return 0;
  }
  if(teofp->frame > 0 && teofp->current >= teofp->frame){
    TeoSetErrorMessage(TEO_ER_OUT_OF_FRAME,
		       teofp->filename,
		       teofp->current,
		       teofp->frame);
    return 0;
  }

  if( TeoBit(teofp) != TeoBit(image) ){
    TeoSetErrorMessage(TEO_ER_CANT_WRITE_FRAME_DIFF_BIT,
		       TeoBit(teofp),TeoBit(image));
    return 0;
  }
  if( (TeoWidth(teofp) != TeoWidth(image)) ||
      (TeoHeight(teofp) != TeoHeight(image)) ||
      (TeoPlane(teofp) != TeoPlane(image)) ){
    TeoSetErrorMessage(TEO_ER_CANT_WRITE_FRAME_DIFF_SIZE,
		       TeoWidth(teofp),TeoHeight(teofp),TeoPlane(teofp),
		       TeoWidth(image),TeoHeight(image),TeoPlane(image));
    return 0;
  }

  teofp->current++;

  if(teofp->bit <= 8){
    if(TeoFwriteEndian((char *)(image)->data,1,
		      (teofp)->fsize,(teofp)->fp) == 0){
      TeoSetErrorMessage(TEO_ER_CANT_WRITE_FRAME,teofp->filename);
      return 0;
    }
  } else {
    if(TeoFwriteEndian((char *)(image)->data,(int)((teofp)->bit/8),
		      (teofp)->width * (teofp)->height * (teofp)->plane,
		      (teofp)->fp) == 0){
      TeoSetErrorMessage(TEO_ER_CANT_WRITE_FRAME,teofp->filename);
      return 0;
    }
  }
  return teofp->current;
}


int TeoSetAbsFrame(TEOFILE *teofp,int frame){
  if(teofp == NULL){
    TeoSetErrorMessage(TEO_ER_ACC_NULL_FILE);
    return 0;
  }
  if(teofp->frame > 0 && 
     (frame >= teofp->frame ||
      frame < 0 )){
    TeoSetErrorMessage(TEO_ER_OUT_OF_FRAME,
		       teofp->filename,
		       frame,
		       teofp->frame);
    return 0;
  }
  if(fseek((teofp)->fp,(teofp)->hsize + (teofp)->fsize * (frame),0) == EOF){
    TeoSetErrorMessage(TEO_ER_SOMETHING_ERROR,teofp->filename);
    return 0;
  }
  teofp->current = frame;
  return 1;
}

int TeoSetRelFrame(TEOFILE *teofp,int frame){
  if(teofp == NULL){
    TeoSetErrorMessage(TEO_ER_ACC_NULL_FILE);
    return 0;
  }
  if(teofp->frame > 0 && 
     (teofp->current + frame >= teofp->frame ||
      teofp->current + frame < 0)){
    TeoSetErrorMessage(TEO_ER_OUT_OF_FRAME,
		       teofp->filename,
		       teofp->current + frame,
		       teofp->frame);
    return 0;
  }
  if(fseek((teofp)->fp,(teofp)->fsize * (frame),1) == EOF){
    TeoSetErrorMessage(TEO_ER_SOMETHING_ERROR,teofp->filename);
    return 0;
  }
  teofp->current += frame;
  return 1;
}

char *TeoGetUserExtension(TEOFILE *teofp,char *keyword){
  int i;
  int size;
  char *tok;
  char *value;
  if(teofp == NULL){
    TeoSetErrorMessage(TEO_ER_ACC_NULL_FILE);
    return 0;
  }
  for(i=0;i<teofp->extc;i++){
    tok = value = teofp->extv[i];
    size = 0;
    while(*value != ' ' && *value != '\t' && *value != '\0'){
      size++;
      value++;
    }
    if(strncmp(tok,keyword,size) == 0){
      while(*value == ' ' || *value == '\t') value++;
      return value;
    }
  }
  return NULL;
}
