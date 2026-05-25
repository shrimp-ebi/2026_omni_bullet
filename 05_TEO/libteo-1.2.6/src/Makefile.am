lib_LTLIBRARIES = libteo.la
libteo_la_SOURCES = define.h endian_free.c error.c error.h fileio.c imalloc.c teo.h teo_debug.c

SOMAJOR = 1
SOMINOR = 4

include_HEADERS = teo.h teo_debug.h
libteo_la_LDFLAGS = -no-undefined -version-info $(SOMAJOR):$(SOMINOR)

INCLUDES = -I.
