dnl Require autoconf version >= 2.57
AC_PREREQ(2.57)


dnl ############# Initialization

AC_INIT([RedStore], [0.1], [njh@aelius.com])

AC_CONFIG_SRCDIR(src/redstore.c)
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(src/config.h)


dnl ############# Compiler and tools Checks

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_RANLIB

AC_C_BIGENDIAN
AC_C_INLINE
AC_C_CONST


dnl ############# Store the build time (as RFC 1123)

BUILD_TIME=`date -u "+%a, %d %b %Y %H:%M:%S GMT"`
AC_DEFINE_UNQUOTED(BUILD_TIME, "$BUILD_TIME", [Time that ./configure was run])


dnl ############# Configure Arguments

AC_ARG_ENABLE(debug,
	  [  --enable-debug=[no/yes]   turn on debugging],
	  [ Debugging="Enabled" 
		AC_DEFINE(DEBUG, 1, [ Define if debugging is enabled. ]) ],
	  [ Debugging="Disabled" ]
)


dnl ############## Compiler and Linker Flags

CFLAGS="$CFLAGS -std=c99"
LDFLAGS="$LDFLAGS"

# If debugging is enabled then make warnings errors
if test "$Debugging" = "Enabled"; then
	CFLAGS="$CFLAGS -g  -Wunused -Wall -Werror -pedantic"
else
	# Optimize flag. 3 is about as high as you can sanely go with GCC3.2.
	CFLAGS="$CFLAGS -O3"
fi


dnl ############## Header and function checks

AC_HEADER_STDC
AC_FUNC_MALLOC
AC_FUNC_REALLOC

AC_CHECK_FUNCS(open_memstream)
if test x$ac_cv_func_open_memstream != xyes; then
    AC_CHECK_FUNC(funopen,,
        AC_MSG_ERROR([neither open_memstream or funopen are available on this system])
    )
fi


dnl ############## Check for packages we depend upon

PKG_CHECK_MODULES(LIBMICROHTTPD, libmicrohttpd >= 0.4.2)
AC_SUBST(LIBMICROHTTPD_CFLAGS)
AC_SUBST(LIBMICROHTTPD_LIBS)

PKG_CHECK_MODULES(REDLAND, redland >= 1.0.7)
AC_SUBST(REDLAND_CFLAGS)
AC_SUBST(REDLAND_LIBS)


AC_OUTPUT([Makefile src/Makefile src/http/Makefile])
