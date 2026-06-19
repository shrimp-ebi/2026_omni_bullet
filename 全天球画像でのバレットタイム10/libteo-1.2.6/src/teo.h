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

  $Id: teo.h,v 2.1.2.4 2002/04/02 06:54:13 tkato Exp $
*/
#ifndef _TEO_H_
#define _TEO_H_

#ifdef __cplusplus
extern "C" {
#endif 

#include <stdio.h>
#include <stddef.h>
/*****************************************************************************
                     Version infomation
******************************************************************************/
/* TEO format version */
#ifndef TEO_VERSION
#define TEO_VERSION 1 /* TEO format version 1 */
#endif
/* TEO Image Library Version Number */
#ifndef TEO_LIB_VERSION
#define TEO_LIB_VERSION "TEO library $Name:  $"
#endif
/*****************************************************************************
                     class definition
******************************************************************************/
/* ------------------------------------------------------------------ */
/* Architecture dependent part. Please edit this part along your site */
/* ------------------------------------------------------------------ */
typedef unsigned char    TEO_BIT;
typedef unsigned char    TEO_UINT8;
typedef signed char      TEO_SINT8;
typedef unsigned short   TEO_UINT16;
typedef signed short     TEO_SINT16;
typedef unsigned int     TEO_UINT32;
typedef signed int       TEO_SINT32;
typedef float            TEO_FLOAT32;
typedef double           TEO_FLOAT64;
/* ------------------------------------------------------------------ */

/* TEOFILE (file infomation for TEO format) */
typedef struct{
  /* TEO format header info */
  int width;                     /* image size(X) */
  int height;                    /* image size(Y) */
  int xoffset;                   /* image offset(X)  */
  int yoffset;                   /* image offset(Y) */
  int type;                      /* pixel type */
  int bit;                       /* pixel size */
  int plane;                     /* number of planes */
  int frame;                     /* number of frames */
  int current;                   /* current frame */

  int extc;                      /* user extention counter */
  char **extv;                   /* user extention pointer */
  /* addtional info */
  int fsize;                     /* size of one frame */

  /* inner info */
  int hsize;                     /* size of header */
  FILE *fp;                      /* file pointer of TEO file */

  int ac_type;                   /* access mode */
  char *filename;                /* file name */
  char *tmpfile;                 /* temporary file name */
} TEOFILE;

/* TEOIMAGE (TEO image class information */
typedef struct{
  /* image info */
  int width;                     /* image size(X) */
  int height;                    /* image size(Y) */
  int xoffset;                   /* image offset(X) */
  int yoffset;                   /* image offset(Y) */
  int type;                      /* pixel type */
  int bit;                       /* pixel size */
  int plane;                     /* number of planes */
  void *data;                    /* pointer of image data */

  /* addtional info */
  int fsize;                     /* size of one frame */
} TEOIMAGE;


/* pixel type */
#define TEO_SIGNED 0   /* signed integer */
#define TEO_UNSIGNED 1 /* unsigned integer */
#define TEO_FLOAT 2    /* floating number */

/* access mode */
#define TEO_AC_READ 1     /* read mode */
#define TEO_AC_WRITE 2    /* write mode */
#define TEO_AC_STDIN 4    /* read from stdin mode */
#define TEO_AC_STDOUT 5   /* write to stdout mode */

#define TEO_AC_TMP_R 6    /* read using tmp file mode */
#define TEO_AC_TMP_W 7    /* write using tmp file mode */
#define TEO_AC_TMP_SW 8   /* write to stdout using tmp file mode */

#define TEO_AC_PIPE_SR 9  /* read from stdin using pipe */
#define TEO_AC_PIPE_R  10 /* read using pipe */
#define TEO_AC_PIPE_W  11 /* write using pipe */

#define TEO_MAXLINE 256
#define TEO_MAX_IMAGE_SIZE (100*1024*1024)

/*****************************************************************************
  error code and error function.
******************************************************************************/

#ifndef __FILE__
#define __FILE__ "unknown"
#endif
#ifndef __LINE__
#define __LINE__ 0
#endif

/* TEO file I/O functions returns NULL and set error code in
   TEO_ERROR_CODE, if something error is found */

/* error management function */
void TeoErrorBase(char *,int);
#define TeoError() TeoErrorBase(__FILE__,__LINE__);
/* variable for error code */
extern int TEO_ERROR_CODE;

/* definition of error codes for TEO file I/O functions */
#define TEO_ER_SOMETHING_ERROR 0

#define TEO_ER_CANT_OPEN 1
#define TEO_ER_CANT_CREATE 2

#define TEO_ER_READ_ERROR 3
#define TEO_ER_WRITE_ERROR 4

#define TEO_ER_READ_WRONG_MAGIC 5
#define TEO_ER_WRITE_WRONG_MAGIC 6

#define TEO_ER_READ_WRONG_VERSION 7
#define TEO_ER_WRITE_WRONG_VERSION 8

#define TEO_ER_READ_WRONG_PTYPE 9
#define TEO_ER_WRITE_WRONG_PTYPE 10

#define TEO_ER_READ_WRONG_PSIZE 11
#define TEO_ER_WRITE_WRONG_PSIZE 12

#define TEO_ER_READ_WRONG_FSIZE 13
#define TEO_ER_WRITE_WRONG_FSIZE 14

#define TEO_ER_OUT_OF_FRAME 15

#define TEO_ER_READ_CANT_GET_FORMAT 16
#define TEO_ER_READ_CANT_GET_SIZE 17
#define TEO_ER_READ_CANT_GET_PTYPE 18
#define TEO_ER_READ_CANT_GET_PLANE 19

#define TEO_ER_CANT_READ_FRAME 20
#define TEO_ER_CANT_WRITE_FRAME 21

#define TEO_ER_CLOSE_WRONG_FRAME 22

#define TEO_ER_NULL_FILENAME 23
#define TEO_ER_TOO_LONG_FILENAME 24
#define TEO_ER_WRONG_FILE_SUFFIX 25


#define TEO_ER_ACC_OUT_OF_XRANGE 26
#define TEO_ER_ACC_OUT_OF_YRANGE 27
#define TEO_ER_ACC_OUT_OF_PRANGE 28
#define TEO_ER_ACC_WRONG_PSIZE 29
#define TEO_ER_ACC_NULL_IMAGE 30

#define TEO_ER_ALLOC_WRONG_TYPE 31
#define TEO_ER_ALLOC_WRONG_PSIZE 32
#define TEO_ER_ALLOC_WRONG_SIZE 33
#define TEO_ER_ALLOC_NOT_ENOUGHT_MEMORY 34

#define TEO_ER_ALLOC_TOO_BIG_IMAGE 35
#define TEO_ER_FREE_NULL_IMAGE 36
#define TEO_ER_FREE_NULL_DATA 37

#define TEO_ER_ACC_NULL_FILE 38

#define TEO_ER_CANT_READ_FRAME_DIFF_SIZE 39
#define TEO_ER_CANT_READ_FRAME_DIFF_BIT 40
#define TEO_ER_CANT_WRITE_FRAME_DIFF_SIZE 41
#define TEO_ER_CANT_WRITE_FRAME_DIFF_BIT 42

#define TEO_ER_CREATE_WRONG_TYPE 43
#define TEO_ER_CREATE_WRONG_PSIZE 44
#define TEO_ER_CREATE_WRONG_SIZE 45
#define TEO_ER_CREATE_NOT_ENOUGHT_MEMORY 46

#define TEO_ER_CREATE_TOO_BIG_IMAGE 47
#define TEO_ERRORS 48

/*****************************************************************************
  TEO inner using and low level file access functions and macros.
******************************************************************************/

/********************* Inner using functions **************************/

/* endian free functions */

/* endian test function */
/* If little endian then it returns 1, otherwise returns 0 */
int TeoIsLittleEndian(void);

/* exchange little endian <-> big endian */
int TeoTransEndian(void *buf, int size);

/* endian free file access functions */
int TeoFreadEndian(void *buf, int size, int num, FILE *fp);
int TeoFwriteEndian(void *buf, int size, int num, FILE *fp);
/* generate temporary file name */
char *TeoGenerateTmp();


/* read TEO header info. */
int TeoReadHeader(TEOFILE *teofp);
/* write TEO header info. */
int TeoWriteHeader(TEOFILE *teofp);

/* Pnm compatible functions */
/* read and write PNM header info. */
int PnmReadHeader(TEOFILE *teofp);
int PnmWriteHeader(TEOFILE *teofp);

/********************* low level functions **************************/

TEOFILE *TeoCreateNonGzFileWithUserExtension(char *filename,
					     int width,int height,
					     int xoffset,int yoffset,
					     int type,int bit,
					     int plane,int frame,
					     int extc,char **extv);

#define TeoCreateNonGzFile(filename,width,height,xoffset,yoffset,type,bit,plane,frame)\
TeoCreateNonGzFileWithUserExtension(filename,width,height,xoffset,yoffset,\
				    type,bit,plane,frame,0,NULL)

#define TeoCreateNonGzSimilarFile(filename,teofp)\
TeoCreateNonGzFileWithUserExtension((filename),\
				    (teofp)->width,(teofp)->height,\
				    (teofp)->xoffset,(teofp)->yoffset,\
				    (teofp)->type,(teofp)->bit,\
				    (teofp)->plane,(teofp)->frame,\
				    (teofp)->extc,(teofp)->extv)

TEOFILE *TeoCreateGzFileWithUserExtension(char *filename,
					   int width,int height,
					   int xoffset,int yoffset,
					   int type,int bit,
					   int plane,int frame,
					   int extc,char **extv);

#define TeoCreateGzFile(filename,width,height,xoffset,yoffset,type,bit,plane,frame)\
TeoCreateGzFileWithUserExtension(filename,width,height,xoffset,yoffset,\
			 type,bit,plane,frame,0,NULL)

#define TeoCreateGzSimilarFile(filename,teofp)\
TeoCreateGzFileWithUserExtension((filename),\
				 (teofp)->width,(teofp)->height,\
				 (teofp)->xoffset,(teofp)->yoffset,\
				 (teofp)->type,(teofp)->bit,\
				 (teofp)->plane,(teofp)->frame,\
				 (teofp)->extc,(teofp)->extv)

TEOFILE *PnmCreateFile(char *filename,int width,int height,
		       int type,int bit,int plane);

#define PnmCreateSimilarFile(filename,teofp)\
PnmCreateFile((filename),\
	  (teofp)->width,(teofp)->height,\
	  (teofp)->type,(teofp)->bit,(teofp)->plane)

TEOFILE *PnmCreateGzFile(char *filename,int width,int height,
		     int type,int bit,int plane);

#define PnmCreateGzSimilarFile(filename,teofp)\
PnmCreateGzFile((filename),\
	    (teofp)->width,(teofp)->height,\
	    (teofp)->type,(teofp)->bit,(teofp)->plane)

TEOFILE *PnmCreateNonGzFile(char *filename,int width,int height,
		     int type,int bit,int plane);

#define PnmCreateNonGzSimilarFile(filename,teofp)\
PnmCreateNonGzFile((filename),\
	    (teofp)->width,(teofp)->height,\
	    (teofp)->type,(teofp)->bit,(teofp)->plane)

/*****************************************************************************
  TEO general using file access functions and macros.
******************************************************************************/
TEOFILE *TeoOpenFile(char *filename);

TEOFILE *TeoCreateFileWithUserExtension(char *filename,
					int width,int height,
					int xoffset,int yoffset,
					int type,int bit,
					int plane,int frame,
					int extc,char **extv);

#define TeoCreateFile(filename,width,height,xoffset,yoffset,type,bit,plane,frame)\
TeoCreateFileWithUserExtension(filename,width,height,xoffset,yoffset,\
			       type,bit,plane,frame,0,NULL)

#define TeoCreateSimilarFile(filename,teofp)\
TeoCreateFileWithUserExtension((filename),\
			       (teofp)->width,(teofp)->height,\
			       (teofp)->xoffset,(teofp)->yoffset,\
			       (teofp)->type,(teofp)->bit,\
			       (teofp)->plane,(teofp)->frame,\
			       (teofp)->extc,(teofp)->extv)

int TeoCloseFile(TEOFILE *teofp);

#define TeoNewSimilarFile(filename,teofp) \
  TeoCreateFileWithUserExtension((filename),\
			       (teofp)->width,(teofp)->height,\
			       (teofp)->xoffset,(teofp)->yoffset,\
			       (teofp)->type,(teofp)->bit,\
			       (teofp)->plane,(teofp)->frame,\
			       (teofp)->extc,(teofp)->extv)
#define TeoNewFile(filename,width,height,xoffset,yoffset,plane,frame,ETYPE) \
TeoNewFileWithUserExtension(filename,width,height,xoffset,yoffset,plane,frame,0,NULL,ETYPE)
#define TeoNewFileWithUserExtension(filename,width,height,xoffset,yoffset,plane,frame,extc,extv,ETYPE) \
TeoNewFileWithUserExtension##ETYPE(filename,width,height,xoffset,yoffset,plane,frame,extc,extv)
#define TeoNewFileWithUserExtensionTEO_BIT(filename,width,height,xoffset,yoffset,plane,frame,extc,extv) \
TeoCreateFile(filename,width,height,xoffset,TEO_UNSIGNE,1,plane,frame,extc,extv)
#define TeoNewFileWithUserExtensionTEO_UINT8(filename,width,height,xoffset,yoffset,plane,frame,extc,extv) \
TeoCreateFile(filename,width,height,xoffset,TEO_UNSIGNED,8,plane,frame,extc,extv)
#define TeoNewFileWithUserExtensionTEO_SINT8(filename,width,height,xoffset,yoffset,plane,frame,extc,extv) \
TeoCreateFile(filename,width,height,xoffset,TEO_SIGNED,8,plane,frame,extc,extv)
#define TeoNewFileWithUserExtensionTEO_UINT16(filename,width,height,xoffset,yoffset,plane,frame,extc,extv) \
TeoCreateFile(filename,width,height,xoffset,TEO_UNSIGNED,16,plane,frame,extc,extv)
#define TeoNewFileWithUserExtensionTEO_SINT16(filename,width,height,xoffset,yoffset,plane,frame,extc,extv) \
TeoCreateFile(filename,width,height,xoffset,TEO_SIGNED,16,plane,frame,extc,extv)
#define TeoNewFileWithUserExtensionTEO_UINT32(filename,width,height,xoffset,yoffset,plane,frame,extc,extv) \
TeoCreateFile(filename,width,height,xoffset,TEO_UNSIGNED,32,plane,frame,extc,extv)
#define TeoNewFileWithUserExtensionTEO_SINT32(filename,width,height,xoffset,yoffset,plane,frame,extc,extv) \
TeoCreateFile(filename,width,height,xoffset,TEO_SIGNED,32,plane,frame,extc,extv)
#define TeoNewFileWithUserExtensionTEO_FLOAT32(filename,width,height,xoffset,yoffset,plane,frame,extc,extv) \
TeoCreateFile(filename,width,height,xoffset,TEO_FLOAT,32,plane,frame,extc,extv)
#define TeoNewFileWithUserExtensionTEO_FLOAT64(filename,width,height,xoffset,yoffset,plane,frame,extc,extv) \
TeoCreateFile(filename,width,height,xoffset,TEO_FLOAT,64,plane,frame,extc,extv)

/* file access functions */
/* read one frame from TEO file */
int TeoReadFrame(TEOFILE *teofp,TEOIMAGE *image);
/* write one frame to TEO file */
int TeoWriteFrame(TEOFILE *teofp,TEOIMAGE *image);

/* file pointer moving functions */
/* move file pointer given frames */
int TeoSetAbsFrame(TEOFILE *teofp,int frame);
int TeoSetRelFrame(TEOFILE *teofp,int frame);

char *TeoGetUserExtension(TEOFILE *teofp,char *keyword);

/* image memory management functions and macros */
TEOIMAGE *TeoAllocImage(int width,int height,int xoffset,int yoffset,
			int type,int bit,int plane);

#define TeoAllocSimilarImage(teofp)\
TeoAllocImage((teofp)->width,(teofp)->height,\
	      (teofp)->xoffset,(teofp)->yoffset,\
	      (teofp)->type,(teofp)->bit,(teofp)->plane)

int TeoFreeImage(TEOIMAGE *image);

/*****************************************************************************
  pixel access functions and macros.
******************************************************************************/

#define TeoGetPixel(image,index_x,index_y,index_p,ETYPE)\
     (*(((ETYPE *)((image)->data)) + \
	(( ((index_y) - (image)->yoffset)*(image)->width +\
	   ((index_x) - (image)->xoffset))*(image)->plane) + (index_p)))

#define TeoPutPixel(image,index_x,index_y,index_p,ETYPE,val)\
     ((*(((ETYPE *)((image)->data)) + \
	 (( ((index_y) - (image)->yoffset)*(image)->width +\
	    ((index_x) - (image)->xoffset))*(image)->plane) + (index_p)))\
      = (ETYPE)(val))

#define TeoGetBit(image,index_x,index_y,index_p)\
     ((((*((TEO_BIT *)(image)->data +\
	   (((index_y)-(image)->yoffset)*\
	    ((int)(((image)->width-1)/8)+1) +\
	    (int)(((index_x)-(image)->xoffset)/8))*\
	   (image)->plane+(index_p)))\
	& (0x01 << (7 - (((index_x)-(image)->xoffset) % 8))))\
       == 0 )? (0) : (1))

#define TeoPutBit(image,index_x,index_y,index_p,val)\
     ((*((TEO_BIT *)(image)->data +\
	 (((index_y)-(image)->yoffset)*\
	  ((int)(((image)->width-1)/8)+1) +\
	  (int)(((index_x)-(image)->xoffset)/8))*\
	 (image)->plane + (index_p)))\
      = (((val) == 0)?\
	 ((*((TEO_UINT8 *)(image)->data +\
	     (((index_y)-(image)->yoffset)*\
	      ((int)(((image)->width-1)/8)+1) +\
	      (int)(((index_x)-(image)->xoffset)/8))*(image)->plane +\
	     (index_p)))\
	  & (~(0x01 << (7-(((index_x)-(image)->xoffset) % 8))))) :\
	 ((*((TEO_UINT8 *)(image)->data +\
	     (((index_y)-(image)->yoffset)*\
	      ((int)(((image)->width-1)/8)+1) +\
	      (int)(((index_x)-(image)->xoffset)/8))*(image)->plane +\
	     (index_p)))\
	  | (0x01 << (7 - (((index_x) - (image)->xoffset) % 8))))))


#define TeoGetPixelValue(image,index_x,index_y,index_p,ETYPE)\
TeoGetPixel##ETYPE(image,index_x,index_y,index_p)
#define TeoPutPixelValue(image,index_x,index_y,index_p,ETYPE,val)\
TeoPutPixel##ETYPE(image,index_x,index_y,index_p,val)

#define TeoGetPixelTEO_BIT(image,index_x,index_y,index_p)\
TeoGetBit(image,index_x,index_y,index_p)
#define TeoPutPixelTEO_BIT(image,index_x,index_y,index_p,val)\
TeoGetBit(image,index_x,index_y,index_p,val)
#define TeoGetPixelTEO_UINT8(image,index_x,index_y,index_p)\
TeoGetPixel(image,index_x,index_y,index_p,TEO_UINT8)
#define TeoPutPixelTEO_UINT8(image,index_x,index_y,index_p,val)\
TeoGetPixel(image,index_x,index_y,index_p,TEO_UINT8,val)
#define TeoGetPixelTEO_SINT8(image,index_x,index_y,index_p)\
TeoGetPixel(image,index_x,index_y,index_p,TEO_SINT8)
#define TeoPutPixelTEO_SINT8(image,index_x,index_y,index_p,val)\
TeoGetPixel(image,index_x,index_y,index_p,TEO_SINT8,val)
#define TeoGetPixelTEO_UINT16(image,index_x,index_y,index_p)\
TeoGetPixel(image,index_x,index_y,index_p,TEO_UINT16)
#define TeoPutPixelTEO_UINT16(image,index_x,index_y,index_p,val)\
TeoGetPixel(image,index_x,index_y,index_p,TEO_UINT16,val)
#define TeoGetPixelTEO_SINT16(image,index_x,index_y,index_p)\
TeoGetPixel(image,index_x,index_y,index_p,TEO_SINT16)
#define TeoPutPixelTEO_SINT16(image,index_x,index_y,index_p,val)\
TeoGetPixel(image,index_x,index_y,index_p,TEO_SINT16,val)
#define TeoGetPixelTEO_UINT32(image,index_x,index_y,index_p)\
TeoGetPixel(image,index_x,index_y,index_p,TEO_UINT32)
#define TeoPutPixelTEO_UINT32(image,index_x,index_y,index_p,val)\
TeoGetPixel(image,index_x,index_y,index_p,TEO_UINT32,val)
#define TeoGetPixelTEO_FLOAT32(image,index_x,index_y,index_p)\
TeoGetPixel(image,index_x,index_y,index_p,TEO_FLOAT32)
#define TeoPutPixelTEO_FLOAT32(image,index_x,index_y,index_p,val)\
TeoGetPixel(image,index_x,index_y,index_p,TEO_FLOAT32,val)
#define TeoGetPixelTEO_FLOAT64(image,index_x,index_y,index_p)\
TeoGetPixel(image,index_x,index_y,index_p,TEO_FLOAT64)
#define TeoPutPixelTEO_FLOAT64(image,index_x,index_y,index_p,val)\
TeoGetPixel(image,index_x,index_y,index_p,TEO_FLOAT64,val)

/* convenience macros */
/* access member of structure TEOFILE or TEOIMAGE */
#define TeoWidth(info)   ((info)->width)
#define TeoHeight(info)  ((info)->height)
#define TeoType(info)    ((info)->type)
#define TeoBit(info)     ((info)->bit)
#define TeoPlane(info)   ((info)->plane)
#define TeoFsize(info)   ((info)->fsize)
#define TeoXoffset(info) ((info)->xoffset)
#define TeoYoffset(info) ((info)->yoffset)

#define TeoXstart(info)  ((info)->xoffset)
#define TeoXend(info)    ((info)->width + (info)->xoffset - 1)
#define TeoYstart(info)  ((info)->yoffset)
#define TeoYend(info)    ((info)->height + (info)->yoffset - 1)

/* access member of structure TEOFILE */
#define TeoFrame(info)   ((info)->frame)
#define TeoHsize(info)   ((info)->hsize)
#define TeoFp(info)      ((info)->fp)
#define TeoCurrent(info) ((info)->current)
#define TeoExtc(info)    ((info)->extc)
#define TeoExtv(info)    ((info)->extv)

/* access member of structure TEOIMAGE */
#define TeoData(info)    ((info)->data)

/* ask pixel type of TEOFILE or TEOIMAGE */
#define TeoIsBIT(info)\
     ((info)->bit == 1)
#define TeoIsUINT8(info)\
     (((info)->bit == 8) && ((info)->type == TEO_UNSIGNED))
#define TeoIsSINT8(info)\
     (((info)->bit == 8) && ((info)->type == TEO_SIGNED))
#define TeoIsUINT16(info)\
     (((info)->bit == 16) && ((info)->type == TEO_UNSIGNED))
#define TeoIsSINT16(info)\
     (((info)->bit == 16) && ((info)->type == TEO_SIGNED))
#define TeoIsUINT32(info)\
     (((info)->bit == 32) && ((info)->type == TEO_UNSIGNED))
#define TeoIsSINT32(info)\
     (((info)->bit == 32) && ((info)->type == TEO_SIGNED))
#define TeoIsFLOAT32(info)\
     (((info)->bit == 32) && ((info)->type == TEO_FLOAT))
#define TeoIsFLOAT64(info)\
     (((info)->bit == 64) && ((info)->type == TEO_FLOAT))

#define TeoCheckPixelType(info,ETYPE)\
TeoIs##ETYPE(info)
#define TeoIsTEO_BIT(info)\
     ((info)->bit == 1)
#define TeoIsTEO_UINT8(info)\
     (((info)->bit == 8) && ((info)->type == TEO_UNSIGNED))
#define TeoIsTEO_SINT8(info)\
     (((info)->bit == 8) && ((info)->type == TEO_SIGNED))
#define TeoIsTEO_UINT16(info)\
     (((info)->bit == 16) && ((info)->type == TEO_UNSIGNED))
#define TeoIsTEO_SINT16(info)\
     (((info)->bit == 16) && ((info)->type == TEO_SIGNED))
#define TeoIsTEO_UINT32(info)\
     (((info)->bit == 32) && ((info)->type == TEO_UNSIGNED))
#define TeoIsTEO_SINT32(info)\
     (((info)->bit == 32) && ((info)->type == TEO_SIGNED))
#define TeoIsTEO_FLOAT32(info)\
     (((info)->bit == 32) && ((info)->type == TEO_FLOAT))
#define TeoIsTEO_FLOAT64(info)\
     (((info)->bit == 64) && ((info)->type == TEO_FLOAT))

#define TeoCheckFrame(info) ((info)->current < (info)->frame)


/* for back compatibility */
/* access member of structure TEOFILE or TEOIMAGE */
#define teo_width(info)   TeoWidth(info)
#define teo_height(info)  TeoHeight(info)
#define teo_type(info)    TeoType(info)
#define teo_bit(info)     TeoBit(info)
#define teo_plane(info)   TeoPlane(info)
#define teo_fsize(info)   TeoFsize(info)
#define teo_xoffset(info) TeoXoffset(info)
#define teo_yoffset(info) TeoYoffset(info)

#define teo_xstart(info)  TeoXstart(info)
#define teo_xend(info)    TeoXend(info)
#define teo_ystart(info)  TeoYstart(info)
#define teo_yend(info)    TeoYend(info)
/* access member of structure TEOFILE */
#define teo_frame(info)   TeoFrame(info)
#define teo_hsize(info)   TeoHsize(info)
#define teo_fp(info)      TeoFp(info)
#define teo_current(info) TeoCurrent(info)
#define teo_extc(info)    TeoExtc(info)
#define teo_extv(info)    TeoExtv(info)
/* access member of structure TEOIMAGE */
#define teo_data(info)    TeoData(info)

#ifdef __cplusplus
}
#endif 
/*****************************************************************************
                     Debug Mode
******************************************************************************/
#if defined(TEO_DEBUG_ALL) || defined(TEO_DEBUG1) || defined(TEO_DEBUG2) || defined(TEO_DEBUG3)
#include <teo_debug.h>
#endif
#endif
