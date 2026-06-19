/*
  $Id: error.h,v 2.1 2001/07/12 02:40:44 tkato Exp $
 */
#ifndef _ERROR_H_
#define _ERROR_H_
struct TEO_ERROR_STRUCT {
  char *strings;
  int level;
} TEO_ERROR[TEO_ERRORS] = {
  {"%s",0},
  /* file io error */
  {"Read error! Can't open file %s.",3},
  {"Write error! Can't create file %s.",3},
  {"Read error! File %s is something wrong.",3},
  {"Write error! Something wrong. (%s)",3},
  {"Read error! %s isn't supported image.",3},
  {"Write error! Not supported image. (%s)",3},
  {"Read error! %s has wrong format version.",3},
  {"Write error! Format version is wrong. (%s)",3},
  {"Read error! %s has wrong pixel type.",3},
  {"Write error! Pixel type is wrong. (%s)",3},
  {"Read error! %s has wrong pixel size.",3},
  {"Write error! Pixel size is wrong. (%s)",3},
  {"Read error! %s has wrong frame size.",3},
  {"Write error! Frame size is wrong. (%s)",3},
  {"Out of frame. (file=%s, frame=%d [0 <= frame < %d] )",3},
  {"Read error! Can't get magic or version. (%s)",3},
  {"Read error! Can't get image size or offset. (%s)",3},
  {"Read error! Can't get pixel type or pixel size. (%s)",3},
  {"Read error! Can't get plane or frame. (%s)",3},
  {"Read error! Can't read frame. (%s)",3},
  {"Write error! Can't write frame. (%s)",3},
  {"Current frame isn't last frame. (frame = %d [last frame = %d])",3},
  {"Filename is null.",3},
  {"Filename is too long. (filename = %s)",2},
  {"Wrong suffix. (filename=%s [ %s ])",2},
  /* image access error */
  {"Out of x range. (x = %d [%d <= x <= %d])",1},
  {"Out of y range. (y = %d [%d <= y <= %d])",1},
  {"Out of plane range. (p = %d [0 <= p < %d])",1},
  {"Wrong pixel size. (pixel size = %d [pixel size = %d])",1},
  {"Can't access. TEOFILE* or TEOIMAGE* is null.",1},
  /* image allocate error */
  {"Can't allocate image, because pixel type is wrong. (type = %d [0(TEO_SIGNED), 1(TEO_UNSIGNED) or 2(TEO_FLOAT)])",1},
  {"Can't allocate image, because pixel size is wrong. (bit = %d [1, 8, 16, 32 or 64(if type is TEO_FLOAT)])",1},
  {"Can't allocate image, because image size is wrong. (width = %d, height = %d, plane = %d)",1},
  {"Can't allocate image, because not enough memory.",1},
  {"Alloc warning. Image size is too big(size=%f). Recommend smaller than %d.",2},
  {"Can't free image, because null image.",1},
  {"Can't free image, because null data.",1},

  {"TEOFILE is null, because you didn't Open(Create) this file.",1},

  {"Can't read frame, because they have different image size. (TEOFILE.{width=%d, height=%d, plane=%d}, TEOIMAGE.{width=%d, height=%d, plane=%d}",1},
  {"Can't read frame, because they have different bit size. (TEOFILE.bit=%d, TEOIMAGE.bit=%d",1},
  {"Can't write frame, because they have different image size. (TEOFILE.{width=%d, height=%d, plane=%d}, TEOIMAGE.{width=%d, height=%d, plane=%d}",1},
  {"Can't write frame, because they have different bit size. (TEOFILE.bit=%d, TEOIMAGE.bit=%d",1},

  {"Can't create image, because pixel type is wrong. (type = %d [0(TEO_SIGNED), 1(TEO_UNSIGNED) or 2(TEO_FLOAT)])",1},
  {"Can't create image, because pixel size is wrong. (bit = %d [1, 8, 16, 32 or 64(if type is TEO_FLOAT)])",1},
  {"Can't create image, because image size is wrong. (width = %d, height = %d, plane = %d, frame = %d)",1},
  {"Can't create image, because not enough memory.",1},
  {"Create warning. Image size is too big(size=%f). Recommend smaller than %d.",2}
};
#endif
