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

  $Id: teo_debug.h,v 2.1.2.1 2001/12/26 05:50:30 tkato Exp $
*/
#ifndef _TEO_DEBUG_H_
#define _TEO_DEBUG_H_
/********************* low level functions **************************/

#if defined(TEO_DEBUG_ALL) && !defined(TEO_DEBUG3)
#define TEO_DEBUG3
#endif
#if defined(TEO_DEBUG3) && !defined(TEO_DEBUG2)
#define TEO_DEBUG2
#endif
#if defined(TEO_DEBUG2) && !defined(TEO_DEBUG1)
#define TEO_DEBUG1
#endif

#ifndef TEO_DEBUG_ERROR
#if defined(TEO_DEBUG3)
#define TEO_DEBUG_FLAG 3
#elif defined(TEO_DEBUG2)
#define TEO_DEBUG_FLAG 2
#elif defined(TEO_DEBUG1)
#define TEO_DEBUG_FLAG 1
#else
#define TEO_DEBUG_FLAG 0
#endif
#else
#if defined(TEO_DEBUG3)
#define TEO_DEBUG_FLAG 7
#elif defined(TEO_DEBUG2)
#define TEO_DEBUG_FLAG 6
#elif defined(TEO_DEBUG1)
#define TEO_DEBUG_FLAG 5
#else
#define TEO_DEBUG_FLAG 4
#endif
#endif

void TeoSetErrorMessage(int,...);
void TeoDebugError(int,char*,int);

TEOFILE *TeoDebugCreateNonGzFileWithUserExtension(char *filename,
						  int width,int height,
						  int xoffset,int yoffset,
						  int type,int bit,
						  int plane,int frame,
						  int extc,char **extv,
						  int flag,char *file,int line);
#define TeoCreateNonGzFileWithUserExtension(filename,\
					    width,height,\
					    xoffset,yoffset,\
					    type,bit,\
					    plane,frame,\
					    extc,extv)\
TeoDebugCreateNonGzFileWithUserExtension(filename,\
					 width,height,\
					 xoffset,yoffset,\
					 type,bit,\
					 plane,frame,\
					 extc,extv,\
					 TEO_DEBUG_FLAG,__FILE__,__LINE__)

TEOFILE *TeoDebugCreateGzFileWithUserExtionsion(char *filename,
						int width,int height,
						int xoffset,int yoffset,
						int type,int bit,
						int plane,int frame,
						int extc,char **extv,
						int flag,char *file,int line);
#define TeoCreateGzFileWithUserExtionsion(filename,\
					  width,height,\
					  xoffset,yoffset,\
					  type,bit,\
					  plane,frame,\
					  extc,extv)\
TeoDebugCreateGzFileWithUserExtionsion(filename,\
				       width,height,\
				       xoffset,yoffset,\
				       type,bit,\
				       plane,frame,\
				       extc,extv,\
				       TEO_DEBUG_FLAG,__FILE__,__LINE__)

TEOFILE *PnmDebugCreateFile(char *filename,int width,int height,
			    int type,int bit,int plane,int flag,char *file,int line);
#define PnmCreateFile(filename,width,height,type,bit,plane)\
TeoDebugCreateFile(filename,width,height,type,bit,plane,TEO_DEBUG_FLAG,__FILE__,__LINE__)

TEOFILE *PnmDebugCreateGzFile(char *filename,int width,int height,
			      int type,int bit,int plane,int flag,char *file,int line);
#define PnmCreateGzFile(filename,width,height,type,bit,plane)\
PnmDebugCreateGzFile(filename,width,height,type,bit,plane,TEO_DEBUG_FLAG,__FILE__,__LINE__)

TEOFILE *PnmDebugCreateNonGzFile(char *filename,int width,int height,
				 int type,int bit,int plane,int flag,char *file,int line);
#define PnmCreateNonGzFile(filename,width,height,type,bit,plane)\
PnmDebugCreateNonGzFile(filename,width,height,type,bit,plane,TEO_DEBUG_FLAG,__FILE__,__LINE__)

/*****************************************************************************
  TEO general using file access functions and macros.
******************************************************************************/
TEOFILE *TeoDebugOpenFile(char *filename,int flag,char *file,int line);
#define TeoOpenFile(filename)\
TeoDebugOpenFile(filename,TEO_DEBUG_FLAG,__FILE__,__LINE__)

TEOFILE *TeoDebugCreateFileWithUserExtension(char *filename,
					     int width,int height,
					     int xoffset,int yoffset,
					     int type,int bit,
					     int plane,int frame,
					     int extc,char **extv,
					     int flag,char *file,int line);
#define TeoCreateFileWithUserExtension(filename,\
				       width,height,\
				       xoffset,yoffset,\
				       type,bit,\
				       plane,frame,\
				       extc,extv)\
TeoDebugCreateFileWithUserExtension(filename,\
				       width,height,\
				       xoffset,yoffset,\
				       type,bit,\
				       plane,frame,\
				       extc,extv,\
				       TEO_DEBUG_FLAG,__FILE__,__LINE__)

int TeoDebugCloseFileNULL(TEOFILE **teofp,int flag,char *file,int line);
#define TeoCloseFile(teofp)\
TeoDebugCloseFileNULL(&teofp,TEO_DEBUG_FLAG,__FILE__,__LINE__)

/* file access functions */
/* read one frame from TEO file */
int TeoDebugReadFrame(TEOFILE *teofp,TEOIMAGE *image,int flag,char *file,int line);
#define TeoReadFrame(teofp,image)\
TeoDebugReadFrame(teofp,image,TEO_DEBUG_FLAG,__FILE__,__LINE__)

/* write one frame to TEO file */
int TeoDebugWriteFrame(TEOFILE *teofp,TEOIMAGE *image,int flag,char *file,int line);
#define TeoWriteFrame(teofp,image)\
TeoDebugWriteFrame(teofp,image,TEO_DEBUG_FLAG,__FILE__,__LINE__)

/* file pointer moving functions */
/* move file pointer given frames */
int TeoDebugSetAbsFrame(TEOFILE *teofp,int frame,int flag,char *file,int line);
#define TeoSetAbsFrame(teofp,frame)\
TeoDebugSetAbsFrame(teofp,frame,TEO_DEBUG_FLAG,__FILE__,__LINE__)

int TeoDebugSetRelFrame(TEOFILE *teofp,int frame,int flag,char *file,int line);
#define TeoSetRelFrame(teofp,frame)\
TeoDebugSetRelFrame(teofp,frame,TEO_DEBUG_FLAG,__FILE__,__LINE__)

char *TeoDebugGetUserExtension(TEOFILE *teofp,char *keyword,int flag,char *file,int line);
#define TeoGetUserExtension(teofp,keyword)\
TeoDebugGetUserExtension(teofp,keyword,TEO_DEBUG_FLAG,__FILE__,__LINE__)

/* image memory management functions and macros */
TEOIMAGE *TeoDebugAllocImage(int width,int height,int xoffset,int yoffset,
			     int type,int bit,int plane,int flag,char *file,int line);
#define TeoAllocImage(width,height,xoffset,yoffset,type,bit,plane)\
TeoDebugAllocImage(width,height,xoffset,yoffset,type,bit,plane,TEO_DEBUG_FLAG,__FILE__,__LINE__)

int TeoDebugFreeImageNULL(TEOIMAGE **image,int flag,char *file,int line);
#define TeoFreeImage(image)\
TeoDebugFreeImageNULL(&image,TEO_DEBUG_FLAG,__FILE__,__LINE__)

#undef TeoGetPixel
#undef TeoPutPixel
#undef TeoGetBit
#undef TeoPutBit

void TeoDebugRangeCheck(TEOIMAGE *image,
			int index_x,
			int index_y,
			int index_p,
			size_t size,
			int flag,char *file,int line);

#define TeoGetPixel(image,index_x,index_y,index_p,ETYPE)\
(TeoDebugRangeCheck(image,index_x,index_y,index_p,\
		    sizeof(ETYPE),TEO_DEBUG_FLAG,__FILE__,__LINE__),\
 (*(((ETYPE *)((image)->data)) + \
    (( ((index_y) - (image)->yoffset)*(image)->width +\
       ((index_x) - (image)->xoffset))*(image)->plane) + (index_p))) )

#define TeoPutPixel(image,index_x,index_y,index_p,ETYPE,val)\
(TeoDebugRangeCheck(image,index_x,index_y,index_p,\
		    sizeof(ETYPE),TEO_DEBUG_FLAG,__FILE__,__LINE__),\
 ((*(((ETYPE *)((image)->data)) + \
     (( ((index_y) - (image)->yoffset)*(image)->width +\
	((index_x) - (image)->xoffset))*(image)->plane) +\
     (index_p)))\
  = (ETYPE)(val)) )

#define TeoPutBit(image,index_x,index_y,index_p,val)\
(TeoDebugRangeCheck(image,index_x,index_y,index_p,\
		    0,TEO_DEBUG_FLAG,__FILE__,__LINE__),\
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
      | (0x01 << (7 - (((index_x) - (image)->xoffset) % 8)))))) )

#define TeoGetBit(image,index_x,index_y,index_p)\
(TeoDebugRangeCheck(image,index_x,index_y,index_p,\
		    0,TEO_DEBUG_FLAG,__FILE__,__LINE__),\
 ((((*((TEO_BIT *)(image)->data +\
       (((index_y)-(image)->yoffset)*\
	((int)(((image)->width-1)/8)+1) +\
	(int)(((index_x)-(image)->xoffset)/8))*\
       (image)->plane+(index_p)))\
    & (0x01 << (7 - (((index_x)-(image)->xoffset) % 8))))\
   == 0 )? (0) : (1)) )

#undef TeoWidth
#undef TeoHeight
#undef TeoType
#undef TeoBit
#undef TeoPlane
#undef TeoFsize
#undef TeoXoffset
#undef TeoYoffset

#undef TeoXstart
#undef TeoXend
#undef TeoYstart
#undef TeoYend

#undef TeoFrame
#undef TeoHsize
#undef TeoFp
#undef TeoCurrent
#undef TeoExtc
#undef TeoExtv

#undef TeoData

#undef TeoIsBIT
#undef TeoIsUINT8
#undef TeoIsSINT8
#undef TeoIsUINT16
#undef TeoIsSINT16
#undef TeoIsUINT32
#undef TeoIsSINT32
#undef TeoIsFLOAT32
#undef TeoIsFLOAT64

#undef TeoCheckFrame

#define TeoWidth(info)   ((info==NULL)? \
			  (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
			   TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
			  ((info)->width))
#define TeoHeight(info)  ((info==NULL)? \
			  (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
			   TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
                          ((info)->height))
#define TeoType(info)    ((info==NULL)? \
			  (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
			   TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
                          ((info)->type))
#define TeoBit(info)     ((info==NULL)? \
			  (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
			   TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
                          ((info)->bit))
#define TeoPlane(info)   ((info==NULL)? \
			  (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
			   TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
                          ((info)->plane))
#define TeoFsize(info)   ((info==NULL)? \
			  (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
			   TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
                          ((info)->fsize))
#define TeoXoffset(info) ((info==NULL)? \
			  (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
			   TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
                          ((info)->xoffset))
#define TeoYoffset(info) ((info==NULL)? \
			  (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
			   TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
                          ((info)->yoffset))

#define TeoXstart(info)  ((info==NULL)? \
			  (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
			   TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
                          ((info)->xoffset))
#define TeoXend(info)    ((info==NULL)? \
			  (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
			   TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
                          ((info)->width + (info)->xoffset - 1))
#define TeoYstart(info)  ((info==NULL)? \
			  (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
			   TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
                          ((info)->yoffset))
#define TeoYend(info)    ((info==NULL)? \
			  (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
			   TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
                          ((info)->height + (info)->yoffset - 1))

/* access member of structure TEOFILE */
#define TeoFrame(info)   ((info==NULL)? \
			  (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
			   TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
                          ((info)->frame))
#define TeoHsize(info)   ((info==NULL)? \
			  (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
			   TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
                          ((info)->hsize))
#define TeoFp(info)      ((info==NULL)? \
			  (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
			   TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
                          ((info)->fp))
#define TeoCurrent(info) ((info==NULL)? \
			  (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
			   TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
                          ((info)->current))
#define TeoExtc(info)    ((info==NULL)? \
			  (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
			   TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
                          ((info)->extc))
#define TeoExtv(info)    ((info==NULL)? \
			  (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
			   TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
                          ((info)->extv))

/* access member of structure TEOIMAGE */
#define TeoData(info)    ((info==NULL)? \
			  (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
			   TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
                          ((info)->data))

/* ask pixel type of TEOFILE or TEOIMAGE */
#define TeoIsBIT(info)\
     ((info==NULL)? \
      (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
       TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
      ((info)->bit == 1))
#define TeoIsUINT8(info)\
     ((info==NULL)? \
      (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
       TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
      (((info)->bit == 8) && ((info)->type == TEO_UNSIGNED)))
#define TeoIsSINT8(info)\
     ((info==NULL)? \
      (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
       TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
      (((info)->bit == 8) && ((info)->type == TEO_SIGNED)))
#define TeoIsUINT16(info)\
     ((info==NULL)? \
      (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
       TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
      (((info)->bit == 16) && ((info)->type == TEO_UNSIGNED)))
#define TeoIsSINT16(info)\
     ((info==NULL)? \
      (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
       TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
      (((info)->bit == 16) && ((info)->type == TEO_SIGNED)))
#define TeoIsUINT32(info)\
     ((info==NULL)? \
      (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
       TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
      (((info)->bit == 32) && ((info)->type == TEO_UNSIGNED)))
#define TeoIsSINT32(info)\
     ((info==NULL)? \
      (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
       TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
      (((info)->bit == 32) && ((info)->type == TEO_SIGNED)))
#define TeoIsFLOAT32(info)\
     ((info==NULL)? \
      (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
       TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
      (((info)->bit == 32) && ((info)->type == TEO_FLOAT)))
#define TeoIsFLOAT64(info)\
     ((info==NULL)? \
      (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
       TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
      (((info)->bit == 64) && ((info)->type == TEO_FLOAT)))

#define TeoCheckFrame(info)\
     ((info==NULL)? \
      (TeoSetErrorMessage(TEO_ER_ACC_NULL_IMAGE),\
       TeoDebugError(TEO_DEBUG_FLAG,__FILE__,__LINE__),0):\
      ((info)->current < (info)->frame))
#endif
