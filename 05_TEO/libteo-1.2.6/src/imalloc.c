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

  $Id: imalloc.c,v 2.1.2.1 2001/12/26 05:50:30 tkato Exp $
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "define.h"
#include "teo.h"

void TeoSetErrorMessage(int code,...);

TEOIMAGE *TeoAllocImage(int width,int height,int xoffset,int yoffset,
                        int type,int bit,int plane){
  TEOIMAGE *image;

  if(type != TEO_SIGNED && type != TEO_UNSIGNED && type != TEO_FLOAT){
    TeoSetErrorMessage(TEO_ER_ALLOC_WRONG_TYPE,type);
    return NULL;
  }

  if(bit != 1 && bit != 8 &&
     bit != 16 && bit != 32 &&
     (!( (type == TEO_FLOAT && bit==64) ))){
    TeoSetErrorMessage(TEO_ER_ALLOC_WRONG_PSIZE,type);
    return NULL;
  }

  if(width <= 0 || height <= 0 || plane <= 0){
    TeoSetErrorMessage(TEO_ER_ALLOC_WRONG_SIZE,width,height,plane);
    return NULL;
  }

  if((image = (TEOIMAGE *)malloc(sizeof(TEOIMAGE))) == NULL){
    TeoSetErrorMessage(TEO_ER_ALLOC_NOT_ENOUGHT_MEMORY);
    return NULL;
  }

  image->width = width;
  image->height = height;
  image->xoffset = xoffset;
  image->yoffset = yoffset;
  image->type = type;
  image->bit = bit;
  image->plane = plane;

  image->fsize = (image->bit == 1)? 
    (((int)((image->width-1)/8) + 1) * image->height * image->plane) : 
    (image->width * image->height * (image->bit/8) * image->plane);

  if((image->data = (void *)calloc(image->fsize,1)) == NULL){
    free(image);
    TeoSetErrorMessage(TEO_ER_ALLOC_NOT_ENOUGHT_MEMORY);
    return NULL;
  }
  return image;
}

int TeoFreeImage(TEOIMAGE *image){
  if(image == NULL){
    TeoSetErrorMessage(TEO_ER_FREE_NULL_IMAGE);
    return 0;
  }
  if(image->data == NULL){
    TeoSetErrorMessage(TEO_ER_FREE_NULL_DATA);
    return 0;
  }
  free(image->data);

  image->data = NULL;
  image->width = image->height =
    image->xoffset = image->yoffset = 
    image->type = image->bit =
    image->plane = image->fsize = 0;

  free(image);
  return 1;
}
