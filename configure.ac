#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.60])
AC_INIT([omxil-core], [0.6.0], [daniel.charles at intel.com])
AC_CONFIG_SRCDIR([ilcore/inc/khronos/openmax/include/OMX_Index.h])
AM_INIT_AUTOMAKE([foreign])
AM_CONFIG_HEADER([config.h])

AC_ARG_ENABLE(omx_debug,
    [AC_HELP_STRING([--enable-omx-debug],
                    [build with extra debug symbols @<:@default=no@:>@])],
    [], [enable_omx_debug="no"])

if test "$enable_omx_debug" = "yes"; then
AC_DEFINE([__ENABLE_DEBUG__], [1], [Defined to 1 if extra debug symbols are compiled])
fi

AC_ARG_ENABLE(libyami,
    AC_HELP_STRING([--enable-libyami],
                   [use libyami @<:@default=yes@:>@]),
   [], [enable_libyami="yes"])

if test "$enable_libyami" = "yes"; then
    AC_DEFINE([__USE_LIBYAMI__], [1], [use libyami])
else
    AC_DEFINE([__USE_LIBYAMI__], [0], [do not use libyami])
fi

AC_ARG_ENABLE(native_buffer,
    AC_HELP_STRING([--enable-native-buffer],
                   [use native buffer @<:@default=yes@:>@]),
   [], [enable_native_buffer="yes"])

if test "$enable_native_buffer" = "yes"; then
    AC_DEFINE([__NATIVE_BUFFER_DEFAULT__], [1], [enable native buffer by default])
else
    AC_DEFINE([__NATIVE_BUFFER_DEFAULT__], [0], [disable native buffer by default])
fi

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdint.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_DISABLE_STATIC

pkgconfigdir=${libdir}/pkgconfig
AC_SUBST(pkgconfigdir)

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset strdup])

AC_CONFIG_FILES([Makefile
                 ilcore/src/Makefile
                 base/src/Makefile
		 utils/src/Makefile
                 pkgconfig/Makefile])
AC_OUTPUT([pkgconfig/libomxil_base.pc
	   pkgconfig/libomxil_utils.pc])
