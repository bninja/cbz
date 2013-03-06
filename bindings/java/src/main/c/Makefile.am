AM_CFLAGS = -I/usr/lib/jvm/java-6-openjdk/include

lib_LTLIBRARIES = libcbzjni.la
libcbzjni_la_SOURCES = com_cbz_Context.c \
                       com_cbz_Node.c \
                       util.c
libcbzjni_la_LDFLAGS = -version-info 0:0:0
libcbzjni_la_CPPFLAGS = $(DEPS_CFLAGS)
libcbzjni_la_LIBADD = $(DEPS_LIBS)
