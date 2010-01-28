dnl Require autoconf version >= 2.60
AC_PREREQ(2.60)


dnl ############# Initialization

AC_INIT([RedStore], [0.1], [njh@aelius.com])
AC_CONFIG_SRCDIR(src/redstore.c)
AC_CONFIG_AUX_DIR(build)
AC_CONFIG_MACRO_DIR(build)
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

AC_CANONICAL_HOST


dnl ############# Compiler and tools Checks

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_RANLIB
AC_PROG_CC_C99

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

# FIXME: make Redstore POSIX compatible
AC_DEFINE(_GNU_SOURCE, 1, [Enable GNU Extensions])

# If debugging is enabled then make warnings errors
if test "$Debugging" = "Enabled"; then
	CFLAGS="$CFLAGS -g -Wall -Werror -pedantic"
	CFLAGS="$CFLAGS -Wunused -Wmissing-prototypes -Wmissing-declarations"
else
	# Optimize flag. 3 is about as high as you can sanely go with GCC3.2.
	CFLAGS="$CFLAGS -O3"
fi


dnl ############## Header and function checks

AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h])
AC_FUNC_MALLOC
AC_FUNC_REALLOC


dnl ############## Check for packages we depend upon

PKG_CHECK_MODULES(REDLAND, redland >= 1.0.7)

PKG_CHECK_MODULES(CHECK, check >= 0.9.4, have_check="yes", have_check="no")
AM_CONDITIONAL(HAVE_CHECK, test x"$have_check" = "xyes")
if test x"$have_check" = "xyes"; then
   AC_DEFINE([HAVE_CHECK], 1, [Define to 1 if check library is available])
fi


dnl ############## Output files

AC_CONFIG_HEADERS([src/config.h])

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	src/http/Makefile
	tests/Makefile
])

AC_OUTPUT
