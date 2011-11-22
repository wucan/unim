dnl  Copyright 2011 Can Wu<wu.canus@gmail.com>
dnl Process this file with autoconf to create configure.

AC_PREREQ([2.57])
AC_INIT(unim,[0.0.1], [http://www.canus.org/xmux],xmux)
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign dist-bzip2])
AM_MAINTAINER_MODE

AM_CONFIG_HEADER(config.h)

AC_PROG_CC
AC_PROG_LIBTOOL

# gtk support
PKG_CHECK_MODULES(gtk, "gtk+-2.0", have_gtk=yes, have_gtk=no)
AM_CONDITIONAL(HAVE_GTK, test "x$have_gtk" = "yes")

PKG_CHECK_MODULES(oauth, "oauth", have_oauth=yes, have_oauth=no)

# json support
PKG_CHECK_MODULES(json, "json-glib-1.0", have_json=yes, have_json=no)

AC_SUBST(VERSION)

AC_OUTPUT([Makefile
           src/Makefile])
