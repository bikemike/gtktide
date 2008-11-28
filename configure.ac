# $Id: configure.ac 2963 2008-02-06 01:13:43Z flaterco $

# Int types macros require autoconf 2.60 or later
AC_PREREQ([2.60])

# FIXME: Update version info on each release
AC_INIT([XTide], [2.10], [dave@flaterco.com])

AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([-Wall -Werror])

AM_CONFIG_HEADER([config.h])

AC_PROG_LIBTOOL

ALL_LINGUAS=""
AM_GLIB_GNU_GETTEXT
AC_PROG_INTLTOOL([0.21])

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

# gtktide configs
PKG_CHECK_MODULES([GTKTIDEDEPS], 
	[gtk+-2.0 cairo libglade-2.0 gnome-vfs-2.0],
	[have_gtk=yes],
	MAEMO[have_gtk=no])
AM_CONDITIONAL(MAEMO, [test x$have_gtk = xyes]) 
AC_SUBST([GTKTIDEDEPS_CFLAGS])
AC_SUBST([GTKTIDEDEPS_LIBS])


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
AC_DEFINE_UNQUOTED([xttpd_user], ["$xttpd_user"], [xttpd user])
AC_DEFINE_UNQUOTED([xttpd_group], ["$xttpd_group"], [xttpd group])
if test ! -z "$webmasteraddr"; then
  AC_DEFINE_UNQUOTED([webmasteraddr], ["$webmasteraddr"], [xttpd webmaster address])
fi

# with options.
AC_ARG_ENABLE([time-workaround],
  [AS_HELP_STRING([--enable-time-workaround],
                  [work around Y2038 problem; disable time zones])],
  [
    AS_IF([test x"$enable_time_workaround" = xyes], [
       AC_MSG_WARN([[Y2038 workaround is enabled; time zones are disabled.]])
       AC_DEFINE([TIME_WORKAROUND],[1],[y2038 workaround is enabled; timezones are disabled.])
    ])
  ])
AC_ARG_ENABLE([gnu-attributes],
  [AS_HELP_STRING([--enable-gnu-attributes],
                  [use with g++ -Wall -Wextra to make warnings smarter])],
  [
    AS_IF([test x"$enable_gnu_attributes" = xyes], [
       AC_MSG_NOTICE([[GNU attributes are enabled.]])
       AC_DEFINE([UseGnuAttributes], 1, [GNU attributes are enabled.])
    ])
  ])
AC_ARG_ENABLE([semicolon-pathsep],
  [AS_HELP_STRING([--enable-semicolon-pathsep],
                  [use ; instead of : to separate names in HFILE_PATH (good idea if they begin with C:\)])],
  [
    AS_IF([test x"$enable_semicolon_pathsep" = xyes], [
       AC_MSG_NOTICE([[Using semicolon as HFILE_PATH separator.]])
       AC_DEFINE([UseSemicolonPathsep], 1, [Using semicolon as HFILE_PATH separator])
    ])
  ])
AC_ARG_ENABLE([local-files],
  [AS_HELP_STRING([--enable-local-files],
                  [locate xtide.conf, .xtide.xml, and .disableXTidedisclaimer files in current working directory])],
  [
    AS_IF([test x"$enable_local_files" = xyes], [
       AC_MSG_NOTICE([[Using local files.]])
       AC_DEFINE([UseLocalFiles], 1, [Using local files.])
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
  [AC_DEFINE(acceptarg3_t, [socklen_t], [type of third argument to accept()])
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
	  [AC_DEFINE(acceptarg3_t, [int], [type of third argument to accept()])
       AC_MSG_RESULT(int)
      ],
	  [AC_DEFINE(acceptarg3_t, [size_t], [type of third argument to accept()])
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
#AC_CHECK_HEADER([tcd.h], [],
#                [AC_MSG_ERROR([cannot find tcd.h; try setting CPPFLAGS.])])
#AC_CHECK_LIB([tcd], [get_tide_db_header], [],
#             [AC_MSG_ERROR([cannot find libtcd; try setting LDFLAGS.])])

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
    AC_DEFINE(USE_PRAGMA_MESSAGE, 1, [VC2008 express edition uses #pragma])
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
  AC_DEFINE(HAVE_LLROUND, 1, [have llround])
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
  AC_DEFINE(HAVE_GOOD_STRFTIME, 1, [vc 2008 express edition does not support strftime %l format])
 ],
 AC_MSG_RESULT([no]))

# libtcd stuff
	AC_SUBST([libtcd_rev_date], ["2007-12-10"])
AC_SUBST([libtcd_major_rev], ["2"])
AC_SUBST([libtcd_minor_rev], ["2"])
# NOTE patchlevel must include leading period if it is provided
AC_SUBST([libtcd_patchlevel], [".3"])

# The following is "bad" because "--enable-feature options should
# never make a feature behave differently or cause one feature to
# replace another.  They should only cause parts of the program to be
# built rather than left out."  COMPAT114 is bad for the same reason.
# Nobody should use it, but if they do, it may as well work like this.

AC_SUBST([def_COMPAT114], ["undef"])
AC_SUBST([ver_COMPAT114], [""])
AC_ARG_ENABLE([COMPAT114],
  [AS_HELP_STRING([--enable-COMPAT114],
                  [for emergency use only:  force API changes to enable old applications to build; break applications expecting the v2 API; disable build of shared library; see libtcd.html])],
  [
    AS_IF([test x"$enable_COMPAT114" = xyes], [
       AC_MSG_WARN([[COMPAT114 is enabled!]])
       AC_MSG_WARN([[This option is for emergency use ONLY!]])
       AC_MSG_WARN([[Forcing API changes to enable old applications to build!]])
       AC_MSG_WARN([[This will BREAK applications expecting the v2 API!]])
       AC_MSG_WARN([[Disabling build of shared library!]])
       AC_MSG_WARN([[Read libtcd.html!]])
       enable_shared=no
       AC_SUBST([def_COMPAT114], ["define"])
       AC_SUBST([ver_COMPAT114], ["-COMPAT114"])
    ])
  ])

# Unfortunately, we need to take this to the next level and generate a
# header file that will work outside of autoconf.  Evil:  relying on
# internals of autoconf macro definitions.

AS_IF([test x"$ac_cv_header_sys_types_h" = xyes],
      [AC_SUBST([sub_systypes], ["#include <sys/types.h>"])],
      [AC_SUBST([sub_systypes], [""])])
AS_IF([test x"$ac_cv_header_inttypes_h" = xyes],
        [AC_SUBST([sub_inttypes], ["#include <inttypes.h>"])],
      [test x"$ac_cv_header_stdint_h" = yes],
        [AC_SUBST([sub_inttypes], ["#include <stdint.h>"])],
        [AC_SUBST([sub_inttypes], [""])])

AS_IF([test x"$ac_cv_c_int8_t" = xyes],
      [AC_SUBST([sub_int8_t], ["int8_t"])],
      [AC_SUBST([sub_int8_t], ["$ac_cv_c_int8_t"])])
AS_IF([test x"$ac_cv_c_int16_t" = xyes],
      [AC_SUBST([sub_int16_t], ["int16_t"])],
      [AC_SUBST([sub_int16_t], ["$ac_cv_c_int16_t"])])
AS_IF([test x"$ac_cv_c_int32_t" = xyes],
      [AC_SUBST([sub_int32_t], ["int32_t"])],
      [AC_SUBST([sub_int32_t], ["$ac_cv_c_int32_t"])])
AS_IF([test x"$ac_cv_c_int64_t" = xyes],
      [AC_SUBST([sub_int64_t], ["int64_t"])],
      [AC_SUBST([sub_int64_t], ["$ac_cv_c_int64_t"])])

AS_IF([test x"$ac_cv_c_uint8_t" = xyes],
      [AC_SUBST([sub_uint8_t], ["uint8_t"])],
      [AC_SUBST([sub_uint8_t], ["$ac_cv_c_uint8_t"])])
AS_IF([test x"$ac_cv_c_uint16_t" = xyes],
      [AC_SUBST([sub_uint16_t], ["uint16_t"])],
      [AC_SUBST([sub_uint16_t], ["$ac_cv_c_uint16_t"])])
AS_IF([test x"$ac_cv_c_uint32_t" = xyes],
      [AC_SUBST([sub_uint32_t], ["uint32_t"])],
      [AC_SUBST([sub_uint32_t], ["$ac_cv_c_uint32_t"])])
AS_IF([test x"$ac_cv_c_uint64_t" = xyes],
      [AC_SUBST([sub_uint64_t], ["uint64_t"])],
      [AC_SUBST([sub_uint64_t], ["$ac_cv_c_uint64_t"])])

# Visual C++ 2008 Express Edition has io.h but not unistd.h.
AC_CHECK_HEADERS([io.h])

# Visual C++ 2008 Express Edition has _chsize but not ftruncate.
AC_MSG_CHECKING([for ftruncate])
AC_LINK_IFELSE(
 [AC_LANG_PROGRAM([[
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_IO_H
#include <io.h>
#endif
                  ]], [[ftruncate (0, 0)]])],
  AC_MSG_RESULT([yes]),
 [AC_MSG_RESULT([no])
  AC_MSG_CHECKING([for _chsize])
  AC_LINK_IFELSE(
   [AC_LANG_PROGRAM([[
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_IO_H
#include <io.h>
#endif
		    ]], [[_chsize (0, 0)]])],
   [
    AC_MSG_RESULT([yes])
    AC_DEFINE(ftruncate, [_chsize],[ vc 2008 express edition has _chsize but not ftruncate.])
   ],
   [AC_MSG_RESULT([no])
    AC_MSG_ERROR([found neither ftruncate nor _chsize.])
   ])
 ])

# Visual C++ 2008 Express Edition uses #pragma message not #warning.
AC_SUBST([def_PRAGMAMESSAGE], ["undef"])
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
    AC_SUBST([def_PRAGMAMESSAGE], ["define"])
   ],
   [AC_MSG_RESULT([no])
   ])
 ])


AC_ARG_WITH(
	[xtide], 
	[AS_HELP_STRING(
		[--without-xtide],
		[don't compile xtide])
	],
	[build_xtide=$withval],
	[build_xtide=yes])

AC_ARG_WITH(
	[xttpd], 
	[AS_HELP_STRING(
		[--without-xttpd],
		[don't compile xttpd])
	],
	[build_xttpd=$withval],
	[build_xttpd=yes])

AC_ARG_WITH(
	[tide], 
	[AS_HELP_STRING(
		[--without-tide],
		[don't compile tide])
	],
	[build_tide=$withval],
	[build_tide=yes])

AC_ARG_WITH(
	[gtktide], 
	[AS_HELP_STRING(
		[--without-gtktide],
		[don't compile gtktide])
	],
	[build_gtktide=$withval],
	[build_gtktide=yes])

AC_ARG_WITH(
	[build-config], 
	[AS_HELP_STRING(
		[--with-build-config],
		[maemo])
	],
	[build_config=$withval],
	[build_config=""])

AM_CONDITIONAL(BUILD_XTIDE, [test x$build_xtide = xyes]) 
AM_CONDITIONAL(BUILD_XTTPD, [test x$build_xttpd = xyes]) 
AM_CONDITIONAL(BUILD_TIDE, [test x$build_tide = xyes]) 
AM_CONDITIONAL(BUILD_GTKTIDE, [test x$build_gtktide = xyes]) 


if test "x$build_config" = "xmaemo"; then 
	AC_DEFINE(MAEMO,1,[maemo build configuration])

# Hildon-libs -> libhildon1 (pkg-config name hildon-1)
	PKG_CHECK_MODULES([HILDON_1], [hildon-1],
		[ AC_DEFINE([HAVE_HILDON_1],[1],[hildon-1 is available]) ],
		[have_hildon1=no]
		[ PKG_CHECK_MODULES([HILDON_LIBS], [hildon-libs],
			[ AC_DEFINE([HAVE_HILDON_LIBS],[1],[hildon-libs is available]) ],
			[have_holdon_libs=no])])

# Hildon-fm -> libhildonfm2 (pkg-config name hildon-fm-2)
	PKG_CHECK_MODULES([HILDON_FM_2], [hildon-fm-2],
		[ AC_DEFINE([HAVE_HILDON_FM_2],[1],[hildonfm2 is available]) ],
		[have_hildon_fm_2=no]
		[PKG_CHECK_MODULES([HILDON_FM], [hildon-fm],
			[ AC_DEFINE([HAVE_HILDON_FM],[1],[hildon-fm is available]) ],
			[have_hildon_fm=no])])

# Libossomime0 -> libhildonmime0
	PKG_CHECK_MODULES([HILDON_MIME], [libhildonmime],
		[ AC_DEFINE([HAVE_HILDON_MIME],[1],[libhildonmime is available]) ],
		[have_hildon_mime=no]
		[PKG_CHECK_MODULES([OSSO_MIME], [libossomime],
			[ AC_DEFINE([HAVE_OSSO_MIME],[1],[libossomime is available]) ],
			[have_osso_mime=no])])

fi

AC_ARG_WITH(
	[xtide-data-dir], 
	[AS_HELP_STRING(
		[--with-xtide-data-dir=DATA_DIR],
		[where DATA_DIR is the directory the data files are to be installed])
	],
	[xtidedatadir=$withval],
	[xtidedatadir=${datadir}/xtide])

AC_ARG_WITH(
	[xtide-harmonics-dir], 
	[AS_HELP_STRING(
		[--with-xtide-harmonics-dir=HARMONICS_DIR],
		[where HARMONICS_DIR is the directory the harmonic files are to be installed])
	],
	[harmonicsdir=$withval],
	[harmonicsdir=${xtidedatadir}/harmonics])

AC_SUBST([xtidedatadir])
AC_SUBST([harmonicsdir])

AC_OUTPUT([
Makefile
data/Makefile
po/Makefile.in
libtcd/Makefile
libtcd/tcd.h
])

echo ======================================================
echo building:
if test "x$build_xtide" = "xyes"; then 
	echo " xtide"
fi
if test "x$build_xttpd" = "xyes"; then 
	echo " xttpd"
fi
if test "x$build_tide" = "xyes"; then 
	echo " tide"
fi
if test "x$build_gtktide" = "xyes"; then 
	echo " gtktide"
fi
echo ======================================================

# Cleanup2006 Done
