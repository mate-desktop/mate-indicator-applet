
AC_INIT(src/applet-main.c)

AC_PREREQ(2.53)

AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(indicator-applet, 2011.12.01)

AM_MAINTAINER_MODE
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES])

IT_PROG_INTLTOOL([0.35.0])
AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_C_O
AC_STDC_HEADERS
AC_PROG_LIBTOOL

AC_SUBST(VERSION)
AC_CONFIG_MACRO_DIR([m4])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

###########################
# Dependencies 
###########################

GTK_REQUIRED_VERSION=2.12
INDICATOR_REQUIRED_VERSION=0.3.22

PKG_CHECK_MODULES(APPLET, gtk+-2.0 >= $GTK_REQUIRED_VERSION
                          x11
                          libmatepanelapplet-3.0
                          indicator-0.4 >= $INDICATOR_REQUIRED_VERSION)
AC_SUBST(APPLET_CFLAGS)
AC_SUBST(APPLET_LIBS)

###########################
# Check to see if we're local
###########################

with_localinstall="no"
AC_ARG_ENABLE(localinstall, AS_HELP_STRING([--enable-localinstall], [install all of the files localy instead of system directories (for distcheck)]), with_localinstall=$enableval, with_localinstall=no)

###########################
# Indicator Info
###########################

if test "x$with_localinstall" = "xyes"; then
	INDICATORDIR="${libdir}/indicators/2/"
	INDICATORICONSDIR="${datadir}/indicator-applet/icons/"
else
	INDICATORDIR=`$PKG_CONFIG --variable=indicatordir indicator`
	INDICATORICONSDIR=`$PKG_CONFIG --variable=iconsdir indicator`
fi

AC_SUBST(INDICATORDIR)
AC_SUBST(INDICATORICONSDIR)

##############################
# Custom Junk
##############################

AC_DEFUN([AC_DEFINE_PATH], [
  test "x$prefix" = xNONE && prefix="$ac_default_prefix"
  test "x$exec_prefix" = xNONE && exec_prefix='${prefix}'
  ac_define_path=`eval echo [$]$2`
  ac_define_path=`eval echo [$]ac_define_path`
  $1="$ac_define_path"
  AC_SUBST($1)
  ifelse($3, ,
    AC_DEFINE_UNQUOTED($1, "$ac_define_path"),
    AC_DEFINE_UNQUOTED($1, "$ac_define_path", $3))
])

###########################
# Internationalization
###########################

GETTEXT_PACKAGE=indicator-applet
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Name of the default gettext domain])
AC_DEFINE_PATH(MATELOCALEDIR, "${datadir}/locale", [locale directory])

ALL_LINGUAS="af am an ar ast az be bg bn br bs ca ca@valencia crh csb cs cv cy da de dv el en_AU en_CA en_GB eo es et eu fa fi fr fur fy gl gv he hi hr hu hy id is it ja ka kk km kn ko ku la lb lt lv mk ml mr ms nb ne nl nn oc pa pl pt_BR pt ro ru sc sd si sk sl sq sr sv ta te th tr ug uk ur vec vi zh_CN zh_HK zh_TW"

AM_GLIB_GNU_GETTEXT

##############################
# ./configure Flags messings
##############################

cflags_set=${CFLAGS+set}
# find the actual value for $prefix that we'll end up with
REAL_PREFIX=
if test "x$prefix" = "xNONE"; then
  REAL_PREFIX=$ac_default_prefix
else
  REAL_PREFIX=$prefix
fi

# Have to go $sysconfdir->$prefix/etc->/usr/local/etc   
# if you actually know how to code shell then fix this :-)
SYSCONFDIR_TMP="$sysconfdir"
old_prefix=$prefix
prefix=$REAL_PREFIX
EXPANDED_SYSCONFDIR=`eval echo $SYSCONFDIR_TMP`
prefix=$old_prefix
AC_SUBST(EXPANDED_SYSCONFDIR)

dnl Save flags to aclocal
ACLOCAL="$ACLOCAL $ACLOCAL_FLAGS"

AC_DEFINE_PATH(PREFIX, "${prefix}", [prefix directory])
AC_DEFINE_PATH(SYSCONFDIR, "${sysconfdir}", [system configuration dir])
AC_DEFINE_PATH(LIBDIR, "${libdir}", [system configuration dir])

###########################
# Files
###########################

AC_OUTPUT([
Makefile
src/Makefile
data/Makefile
po/Makefile.in
])

###########################
# Results
###########################

AC_MSG_NOTICE([

Indicator Applet Configuration:

	Prefix:                 $prefix
])
