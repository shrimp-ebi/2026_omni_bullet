AC_DEFUN(AM_PATH_TEO,
[AC_MSG_CHECKING(if teo library wanted)
  AC_ARG_ENABLE(teo, [ --disable-teo     Do no use TEO library],,
		enable_teo=yes)
  if test "x$enable_teo" = "xyes" ; then
    AC_ARG_WITH(teo-prefix,
      [  --with-teo-prefix=PREFIX
         Prefix where TEO installed [default=/usr/local/lab].],
      [teo_prefix=$withval],[teo_prefix=yes])
    if test "${teo_prefix}" = yes ; then
      teo_prefix=/usr/local/lab
    fi
    AC_MSG_RESULT($teo_prefix)
 
    TEO_CFLAGS="-I${teo_prefix}/include"
    TEO_LIBS="-L${teo_prefix}/lib"

    AC_LANG_SAVE
    AC_LANG_C
    ac_save_CFLGAS="$CFLAGS"
    ac_save_LIBS="$LIBS"
    CFLAGS="$CFLAGS $TEO_CFLAGS"
    LIBS="$LIBS $TEO_LIBS"
    AC_CHECK_LIB(teo,TeoOpenFile,TEO_LIBS="${TEO_LIBS} -lteo")
    if test "$ac_cv_lib_teo_TeoOpenFile" != "yes"; then
      echo "Fatal error: no teo library in suggested path ${teo_prefix}/lib"
      exit 1
    fi
    CFLAGS="$ac_save_CFLAGS"
    LIBS="$ac_save_LIBS"
    AC_LANG_RESTORE

  fi
  AC_SUBST(teo_prefix)
  AC_SUBST(TEO_CFLAGS)
  AC_SUBST(TEO_LIBS)
])
