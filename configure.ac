# $Id: configure.ac 2963 2008-02-06 01:13:43Z flaterco $

# Int types macros require autoconf 2.60 or later
AC_PREREQ([2.60])

# FIXME: Update version info on each release
AC_INIT([XTide], [2.10], [dave@flaterco.com])

AM_INIT_AUTOMAKE([-Wall -Werror])

AC_LANG([C++])
AC_PROG_CXX
AM_PROG_LEX
AC_PROG_YACC

# This tests for the presence of a fault that prevents GCC 3.3.6 from
# compiling HarmonicsFile.cc.
AC_MSG_CHECKING([whether C++ compiler is worthy])
AC_COMPILE_IFELSE(
  [
    class ChuckingRate {public: ChuckingRate (const char *rate)           {}};
    class Woodchuck    {public: Woodchuck    (const ChuckingRate &rate)   {}};
    class Groundhog    {public: Groundhog    (const Woodchuck &woodchuck) {}};
    Groundhog phil (Woodchuck (ChuckingRate ("About a cord a day")));
  ],
  [
    AC_MSG_RESULT(yes)
  ],
  [
    AC_MSG_RESULT(no)
    AC_MSG_NOTICE([if using GCC, version 3.4 or later is required])
    AC_MSG_ERROR([compiler found unworthy])
  ]
)

# Ensure that STL is present.  (Reportedly not found without CPPFLAGS
# under Mac OS X 10.4.9)
AC_CHECK_HEADER([map], [], [
  AC_MSG_NOTICE([[Not finding the Standard Template Library.]])
  AC_MSG_NOTICE([[Find a directory like /usr/include/c++/[...numbers...],]])
  AC_MSG_NOTICE([[then do ./configure CPPFLAGS="-I/usr/include/...".]])
  AC_MSG_ERROR([cannot find Standard Template Library; try setting CPPFLAGS.])
])

# xttpd configs
AC_ARG_WITH([xttpd-user],
            [AS_HELP_STRING([--with-xttpd-user=user],
              [user that xttpd should run as])],
            [xttpd_user=$withval],
            [xttpd_user=nobody])
AC_ARG_WITH([xttpd-group],
            [AS_HELP_STRING([--with-xttpd-group=group],
              [group that xttpd should run as])],
            [xttpd_group=$withval],
            [xttpd_group=nobody])
AC_ARG_WITH([webmaster],
            [AS_HELP_STRING([--with-webmaster=addr],
              [email address for xttpd feedback])],
            [webmasteraddr=$withval])
AC_DEFINE_UNQUOTED([xttpd_user], ["$xttpd_user"])
AC_DEFINE_UNQUOTED([xttpd_group], ["$xttpd_group"])
if test ! -z "$webmasteraddr"; then
  AC_DEFINE_UNQUOTED([webmasteraddr], ["$webmasteraddr"])
fi

# with options.
AC_ARG_ENABLE([time-workaround],
  [AS_HELP_STRING([--enable-time-workaround],
                  [work around Y2038 problem; disable time zones])],
  [
    AS_IF([test "$enable_time_workaround" == yes], [
       AC_MSG_WARN([[Y2038 workaround is enabled; time zones are disabled.]])
       AC_DEFINE([TIME_WORKAROUND])
    ])
  ])
AC_ARG_ENABLE([gnu-attributes],
  [AS_HELP_STRING([--enable-gnu-attributes],
                  [use with g++ -Wall -Wextra to make warnings smarter])],
  [
    AS_IF([test "$enable_gnu_attributes" == yes], [
       AC_MSG_NOTICE([[GNU attributes are enabled.]])
       AC_DEFINE([UseGnuAttributes])
    ])
  ])
AC_ARG_ENABLE([semicolon-pathsep],
  [AS_HELP_STRING([--enable-semicolon-pathsep],
                  [use ; instead of : to separate names in HFILE_PATH (good idea if they begin with C:\)])],
  [
    AS_IF([test "$enable_semicolon_pathsep" == yes], [
       AC_MSG_NOTICE([[Using semicolon as HFILE_PATH separator.]])
       AC_DEFINE([UseSemicolonPathsep])
    ])
  ])
AC_ARG_ENABLE([local-files],
  [AS_HELP_STRING([--enable-local-files],
                  [locate xtide.conf, .xtide.xml, and .disableXTidedisclaimer files in current working directory])],
  [
    AS_IF([test "$enable_local_files" == yes], [
       AC_MSG_NOTICE([[Using local files.]])
       AC_DEFINE([UseLocalFiles])
    ])
  ])

# For Solaris, this should put -lsocket -lnsl in X_EXTRA_LIBS.
AC_PATH_XTRA
if test "$no_x" = "yes"; then
  AC_MSG_WARN([Type 'make tide' or 'make xttpd' to compile without X11.])
fi

AC_MSG_CHECKING([type of third argument to accept()])
AC_COMPILE_IFELSE(
  [
#include <sys/types.h>
#include <sys/socket.h>
int main (int argc, char **argv) {
  socklen_t length;
  struct sockaddr addr;
  int listen_socket;
  accept (listen_socket, &addr, &length);
  return 0;
}
  ],
  [AC_DEFINE(acceptarg3_t, socklen_t)
   AC_MSG_RESULT(socklen_t)
  ],
  [
    AC_COMPILE_IFELSE(
      [
#include <sys/types.h>
#include <sys/socket.h>
int main (int argc, char **argv) {
  int length;
  struct sockaddr addr;
  int listen_socket;
  accept (listen_socket, &addr, &length);
  return 0;
}
      ],
      [AC_DEFINE(acceptarg3_t, int)
       AC_MSG_RESULT(int)
      ],
      [AC_DEFINE(acceptarg3_t, size_t)
       AC_MSG_RESULT(size_t)
      ]
    )
])

LIBS="$LIBS -lm"

# libz is required by libpng.
AC_CHECK_HEADER([zlib.h], [],
                [AC_MSG_ERROR([cannot find zlib.h; try setting CPPFLAGS.])])
AC_CHECK_LIB([z], [zlibVersion], [],
             [AC_MSG_ERROR([cannot find libz; try setting LDFLAGS.])])

# libpng 0.96 or compatible newer version is required.
AC_CHECK_HEADER([png.h], [],
                [AC_MSG_ERROR([cannot find png.h; try setting CPPFLAGS.])])
AC_CHECK_LIB([png], [png_check_sig], [],
             [AC_MSG_ERROR([cannot find libpng; try setting LDFLAGS.])])

# libtcd 2.2 or compatible newer version is required.
AC_CHECK_HEADER([tcd.h], [],
                [AC_MSG_ERROR([cannot find tcd.h; try setting CPPFLAGS.])])
AC_CHECK_LIB([tcd], [get_tide_db_header], [],
             [AC_MSG_ERROR([cannot find libtcd; try setting LDFLAGS.])])

# libdstr is optional; use it if you have it.
AC_CHECK_HEADER([Dstr], [AC_CHECK_LIB([dstr], [DstrCompat20070215Check])])
AM_CONDITIONAL(NO_LIBDSTR, test "$ac_cv_lib_dstr_DstrCompat20070215Check" != "yes")

# Portable, well-defined int types.  Finally.

AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T

AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Visual C++ 2008 Express Edition lacks some standard headers and requires
# some nonstandard ones.
AC_CHECK_HEADERS([syslog.h langinfo.h dirent.h sys/resource.h io.h process.h])

# Visual C++ 2008 Express Edition uses #pragma message not #warning.
AC_MSG_CHECKING([[if #warning works]])
AC_COMPILE_IFELSE(
 [AC_LANG_PROGRAM([[]], [[#warning warning]])],
  AC_MSG_RESULT([yes]),
 [AC_MSG_RESULT([no])
  AC_MSG_CHECKING([[if #pragma message works]])
  AC_COMPILE_IFELSE(
   [AC_LANG_PROGRAM([[]], [[#pragma message("warning")]])],
   [
    AC_MSG_RESULT([yes])
    AC_DEFINE(USE_PRAGMA_MESSAGE)
   ],
   [AC_MSG_RESULT([no])
   ])
 ])

# Choices are dirent or WIN32-specific code; there is no third option.
AC_MSG_CHECKING([for either dirent or _finddata_t])
AC_COMPILE_IFELSE(
 [AC_LANG_PROGRAM([[
#ifndef HAVE_DIRENT_H
#ifdef HAVE_IO_H
#include <io.h>
#endif
_finddata_t fileinfo;
#endif
]], [[]])],
  AC_MSG_RESULT([yes]),
 [AC_MSG_RESULT([no])
  AC_MSG_ERROR([neither SUSv3 nor WIN32 dirent facility found])
 ])

# Cygwin has lround but not llround.
# Visual C++ 2008 Express Edition has neither.
AC_MSG_CHECKING([for llround])
AC_LINK_IFELSE(
 [AC_LANG_PROGRAM([[#include <math.h>]], [[llround (0)]])],
 [AC_MSG_RESULT([yes])
  AC_DEFINE(HAVE_LLROUND)
 ],
 AC_MSG_RESULT([no]))

# Visual C++ 2008 Express Edition does not support strftime %l format.
AC_MSG_CHECKING([for strftime %l format])
AC_RUN_IFELSE(
 [AC_LANG_PROGRAM([[#include <time.h>]], [[
  char s[3];
  struct tm t1;
  size_t stret;
  t1.tm_hour = 5;
  s[0] = s[1] = 'X';
  stret = strftime (s, 3, "%l", &t1);
  return (stret == 0 || s[0] != ' ' || s[1] != '5');
]])],
 [AC_MSG_RESULT([yes])
  AC_DEFINE(HAVE_GOOD_STRFTIME)
 ],
 AC_MSG_RESULT([no]))

AC_OUTPUT(Makefile)

# Cleanup2006 Done
