dnl Require autoconf version >= 2.59
AC_PREREQ(2.59)


dnl ############# Initialization

AC_INIT([RedStore], [0.2], [njh@aelius.com])
AC_CONFIG_SRCDIR(src/redstore.c)
AC_CONFIG_AUX_DIR(build)
AC_CONFIG_MACRO_DIR(build)
AM_INIT_AUTOMAKE
LT_INIT

dnl ############# Configure Arguments

AC_ARG_ENABLE(debug,
	  [  --enable-debug=[no/yes]   turn on debugging],
	  [ Debugging="Enabled" 
		AC_DEFINE(DEBUG, 1, [ Define if debugging is enabled. ]) ],
	  [ Debugging="Disabled" ]
)


dnl ############## Compiler and Linker Flags

# FIXME: use AC PROG CC C99
CFLAGS="$CFLAGS -std=c99"

# If debugging is enabled then make warnings errors
if test "$Debugging" = "Enabled"; then
	CFLAGS="$CFLAGS -g -O0 -Wall -Werror -pedantic"
	CFLAGS="$CFLAGS -Wunused -Wmissing-prototypes -Wmissing-declarations"
else
	# Optimize flag. 3 is about as high as you can sanely go with GCC3.2.
	CFLAGS="$CFLAGS -O3"
fi


dnl ############# Compiler and tools Checks

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_LIBTOOL

AC_C_BIGENDIAN
AC_C_INLINE
AC_C_CONST


dnl ############# Store the build time (as RFC 1123)

BUILD_TIME=`date -u "+%a, %d %b %Y %H:%M:%S GMT"`
AC_DEFINE_UNQUOTED(BUILD_TIME, "$BUILD_TIME", [Time that ./configure was run])


dnl ############## Header and function checks

AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h])
AC_FUNC_MALLOC
AC_FUNC_REALLOC


dnl ############## Check for packages we depend upon

PKG_CHECK_MODULES(REDLAND, redland >= 1.0.9)

PKG_CHECK_MODULES(CHECK, check >= 0.9.4, have_check="yes", have_check="no")
if test x"$have_check" = "xyes"; then
	AC_CHECK_PROG(have_checkmk, [checkmk], [yes], [no])
	if test x"$have_checkmk" = "xyes"; then
		AC_DEFINE([HAVE_CHECK], 1, [Define to 1 if check library is available])
	else
		AC_MSG_WARN([Command 'checkmk' not found.])
		AC_MSG_WARN([Download it here: http://micah.cowan.name/projects/checkmk/])
	fi
fi
AM_CONDITIONAL(HAVE_CHECK, test x"$have_check" = "xyes" &&
                           test x"$have_checkmk" = "xyes")


dnl ############## Output files

AC_CONFIG_HEADERS([src/config.h])

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	src/redhttp/Makefile
	tests/Makefile
	tests/redhttp/Makefile
])

AC_OUTPUT
