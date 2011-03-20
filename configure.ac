dnl Require autoconf version >= 2.60
AC_PREREQ(2.60)


dnl ############# Initialization

AC_INIT([RedStore], [0.5.4], [njh@aelius.com], redstore)

AC_CONFIG_SRCDIR([src/redstore.c])
AC_CONFIG_AUX_DIR(build-scripts)
AC_CONFIG_MACRO_DIR(build-scripts)
AM_INIT_AUTOMAKE([1.10])

LT_PREREQ([2.2])
LT_INIT

AM_MAINTAINER_MODE



dnl ############# Compiler and tools Checks

AC_PROG_CC
AC_PROG_CC_C99
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

AC_CHECK_FUNCS_ONCE([srandomdev])
if test x"$ac_cv_func_srandomdev" = "xyes"; then
  AC_DEFINE(HAVE_SRANDOMDEV, 1, [Define to 1 if you have srandomdev()].)
fi

dnl ############## Check for packages we depend upon

PKG_CHECK_MODULES(RAPTOR, raptor2 >= 2.0.1)
PKG_CHECK_MODULES(RASQAL, rasqal >= 0.9.25)
PKG_CHECK_MODULES(REDLAND, redland >= 1.0.14)

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


dnl ############## Compiler and Linker Flags

# If maintainer mode is enabled then make warnings errors
if test "$USE_MAINTAINER_MODE" = yes; then
  WARNING_CFLAGS="-Wall -Werror -pedantic -Wunused"
  WARNING_CFLAGS="$WARNING_CFLAGS -Wmissing-prototypes -Wmissing-declarations"
  AC_SUBST(WARNING_CFLAGS)
fi



dnl ############## Output files

AC_CONFIG_HEADERS([src/redstore_config.h])

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  src/redhttp/Makefile
  tests/Makefile
  tests/integration/Makefile
  tests/redhttp/Makefile
])

AC_OUTPUT
