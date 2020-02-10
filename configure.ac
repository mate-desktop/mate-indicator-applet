AC_PREREQ(2.53)

AC_INIT([mate-indicator-applet], [1.24.0],[https://github.com/mate-desktop/mate-indicator-applet/issues])
AC_CONFIG_SRCDIR(src/applet-main.c)

AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([no-dist-gzip dist-xz check-news])

AM_MAINTAINER_MODE
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES])

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

GTK_REQUIRED_VERSION=3.22.0
PANEL_REQUIRED_VERSION=1.17.0

UBUNTU_INDICATOR_REQUIRED_VERSION=0.3.90
UBUNTU_INDICATOR_NG_VERSION=12.10.2

AYATANA_INDICATOR_REQUIRED_VERSION=0.6.0
AYATANA_INDICATOR_NG_VERSION=0.6.0

PKG_CHECK_MODULES(APPLET, gtk+-3.0 >= $GTK_REQUIRED_VERSION
                          x11
                          libmatepanelapplet-4.0 >= $PANEL_REQUIRED_VERSION
                          )

AC_SUBST(APPLET_CFLAGS)
AC_SUBST(APPLET_LIBS)

# Which Indicator implementation shall we use?

AC_ARG_WITH([ayatana-indicators],
             [AS_HELP_STRING([--with-ayatana-indicators],
                             [build against Ayatana Indicators])],
             [with_ayatana_indicators='yes'],
             [with_ayatana_indicators='no']
)

AC_ARG_WITH([ubuntu-indicators],
             [AS_HELP_STRING([--with-ubuntu-indicators],
             [build against Ubuntu Indicators])],
             [with_ubuntu_indicators='yes'],
             [with_ubuntu_indicators='no']
)

###
### Look for Ayatana Indicators
###

PKG_CHECK_EXISTS(ayatana-indicator3-0.4,
                 [have_ayatanaindicator="yes"],
                 [have_ayatanaindicator="no"])

PKG_CHECK_EXISTS(ayatana-indicator3-0.4 >= $AYATANA_INDICATOR_NG_VERSION,
                 [have_ayatanaindicator_ng="yes"],
                 [have_ayatanaindicator_ng="no"])

###
### Look for Ubuntu Indicators
###

PKG_CHECK_EXISTS(indicator3-0.4,
                 [have_ubuntuindicator="yes"],
                 [have_ubuntuindicator="no"])

PKG_CHECK_EXISTS(indicator3-0.4 >= $UBUNTU_INDICATOR_NG_VERSION,
                 [have_ubuntuindicator_ng="yes"],
                 [have_ubuntuindicator_ng="no"])

### decide on what Indicators implementation to use...

if   test "x$have_ayatanaindicator" == "xyes" &&
     test "x$have_ubuntuindicator" != "xyes" &&
     test "x$with_ubuntu_indicators" != "xyes"; then

    # use Ayatana Indicators (because they are present, and noone is enforcing Ubuntu Indicators)
    use_ayatanaindicator="yes";
    indicator_enforced="no";

elif test "x$have_ubuntuindicator" == "xyes" &&
     test "x$have_ayatanaindicator" != "xyes" &&
     test "x$with_ayatana_indicators" != "xyes"; then

    # use Ubuntu Indicators (because they are present, and noone is enforcing Ayatana Indicators)
    use_ubuntuindicator="yes";
    indicator_enforced="no";

elif test "x$have_ubuntuindicator" == "xyes" &&
     test "x$have_ayatanaindicator" == "xyes" &&
     test "x$with_ayatana_indicators" == "xyes"; then

    # both Indicator implementations are present, and we are asked to use Ayatana Indicators
    use_ayatanaindicator=yes;
    indicator_enforced=yes;

elif test "x$have_ubuntuindicator" == "xyes" &&
     test "x$have_ayatanaindicator" == "xyes" &&
     test "x$with_ubuntu_indicators" == "xyes"; then

    # both Indicator implementations are present, and we are asked to use Ubuntu Indicators
    use_ubuntuindicator=yes;
    indicator_enforced=yes;

elif test "x$have_ubuntuindicator" == "xyes" &&
     test "x$have_ayatanaindicator" != "xyes" &&
     test "x$with_ayatana_indicators" == "xyes"; then

    AC_MSG_ERROR([Ubuntu Indicators are present, but you want to build mate-indicator-applet against Ayatana Indicators. This does not match.])

elif test "x$have_ubuntuindicator" != "xyes" &&
     test "x$have_ayatanaindicator" == "xyes" &&
     test "x$with_ubuntu_indicators" == "xyes"; then

    AC_MSG_ERROR([Ayatana Indicators are present, but you want to build mate-indicator-applet against Ubuntu Indicators. This does not match.])

else

    AC_MSG_ERROR([Either Ayatana Indicators or Ubuntu Indicators are required to build mate-indicator-applet.])

fi

### prepare Ayatana or Ubuntu Indicators implementation for the build, regarding to the decision reached above...

if   test "x$use_ayatanaindicator" == "xyes"; then

    AM_CONDITIONAL(WITH_AYATANA_INDICATOR, true)
    AM_CONDITIONAL(WITH_UBUNTU_INDICATOR, false)
    AC_DEFINE(HAVE_AYATANA_INDICATOR, 1, "Ayatana Indicators Support")

    if test "x$indicator_enforced" == "xyes"; then
        AC_MSG_NOTICE([Using Ayatana Indicators for this build (as requested via configure option).])
    else
        AC_MSG_NOTICE([Using Ayatana Indicators for this build.])
    fi

    if test "x$have_ayatanaindicator_ng" = "xyes"; then
        PKG_CHECK_MODULES(AYATANA_INDICATOR_NG, ayatana-indicator3-0.4 >= $AYATANA_INDICATOR_NG_VERSION
                          libayatana-ido3-0.4 >= 0.4.0,
                          [AC_DEFINE(HAVE_AYATANA_INDICATOR_NG, 1, "New style indicators support")])
    elif test "x$have_ayatanaindicator" = "xyes"; then
        PKG_CHECK_MODULES(AYATANA_INDICATOR, ayatana-indicator3-0.4 >= $AYATANA_INDICATOR_REQUIRED_VERSION)
    fi

    AC_SUBST(AYATANA_INDICATOR_CFLAGS)
    AC_SUBST(AYATANA_INDICATOR_LIBS)

elif test "x$use_ubuntuindicator" == "xyes"; then

    # both Indicator implementations are present, and we are asked to use Ubuntu Indicators
    AM_CONDITIONAL(WITH_UBUNTU_INDICATOR, true)
    AM_CONDITIONAL(WITH_AYATANA_INDICATOR, false)
    AC_DEFINE(HAVE_UBUNTU_INDICATOR, 1, "Ubuntu Indicators Support")

    if test "x$indicator_enforced" == "xyes"; then
        AC_MSG_NOTICE([Using Ubuntu Indicators for this build (as requested via configure option).])
    else
        AC_MSG_NOTICE([Using Ubuntu Indicators for this build.])
    fi

    if test "x$have_ubuntuindicator_ng" = "xyes"; then
        PKG_CHECK_MODULES(UBUNTU_INDICATOR_NG, indicator3-0.4 >= $UBUNTU_INDICATOR_NG_VERSION
                          libido3-0.1 >= 13.10,
                          [AC_DEFINE(HAVE_UBUNTU_INDICATOR_NG, 1, "New style indicators support")])
    elif test "x$have_ubuntuindicator" = "xyes"; then
        PKG_CHECK_MODULES(UBUNTU_INDICATOR, indicator3-0.4 >= $UBUNTU_INDICATOR_REQUIRED_VERSION)
    fi

    AC_SUBST(UBUNTU_INDICATOR_CFLAGS)
    AC_SUBST(UBUNTU_INDICATOR_LIBS)

fi

###########################
# Check to see if we're local
###########################

with_localinstall="no"
AC_ARG_ENABLE(localinstall, AS_HELP_STRING([--enable-localinstall], [install all of the files localy instead of system directories (for distcheck)]), with_localinstall=$enableval, with_localinstall=no)

###########################
# Indicator Info
###########################

if test "x$use_ubuntuindicator" = "xyes"; then
	if test "x$with_localinstall" = "xyes"; then
		INDICATORDIR="${libdir}/indicators3/7/"
		INDICATORICONSDIR="${datadir}/indicator-applet/icons/"
	else
		INDICATORDIR=`$PKG_CONFIG --variable=indicatordir indicator3-0.4`
		INDICATORICONSDIR=`$PKG_CONFIG --variable=iconsdir indicator3-0.4`
	fi
fi

if test "x$use_ayatanaindicator" = "xyes"; then
	if test "x$with_localinstall" = "xyes"; then
		INDICATORDIR="${libdir}/ayatana-indicators3/7/"
		INDICATORICONSDIR="${datadir}/ayatana-indicator-applet/icons/"
	else
		INDICATORDIR=`$PKG_CONFIG --variable=indicatordir ayatana-indicator3-0.4`
		INDICATORICONSDIR=`$PKG_CONFIG --variable=iconsdir ayatana-indicator3-0.4`
	fi
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
AM_GNU_GETTEXT_VERSION([0.19.8])
AM_GNU_GETTEXT([external])

GETTEXT_PACKAGE=mate-indicator-applet
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Name of the default gettext domain])
AC_DEFINE_PATH(MATELOCALEDIR, "${datadir}/locale", [locale directory])

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

if (test "x$use_ayatanaindicator" == "xyes" &&
    test "x$have_ayatanaindicator_ng" == "xyes") ||
   (test "x$use_ubuntuindicator" == "xyes" &&
    test "x$have_ubuntuindicator_ng" == "xyes"); then
    have_indicator_ng="yes"
else
    have_indicator_ng="no"
fi

if test "x$use_ayatanaindicator" == "xyes"; then
    indicator_implementation="Ayatana Indicators"
elif test "x$use_ubuntuindicator" == "xyes"; then
    indicator_implementation="Ubuntu Indicators"
fi

AC_MSG_NOTICE([

Indicator Applet Configuration:

	Prefix:                         $prefix
	Indicator implementation:       $indicator_implementation
	Indicator NG support:           $have_indicator_ng
	Indicator Directory:            $INDICATORDIR
	Indicator Icons Directory:      $INDICATORICONSDIR
])
