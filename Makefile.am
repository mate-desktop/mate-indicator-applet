SUBDIRS = \
	src \
	data \
	po

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

DISTCHECK_CONFIGURE_FLAGS = \
	--enable-localinstall \
	--enable-compile-warnings=no \
	CFLAGS='-Wno-deprecated-declarations'

DISTCLEANFILES = \
	indicator-applet-*.tar.gz \
	gtk-doc.make

EXTRA_DIST = \
	autogen.sh \
	COPYING.LGPL

# Build ChangeLog from GIT  history
ChangeLog:
	$(AM_V_GEN) if test -d $(top_srcdir)/.git; then \
		GIT_DIR="$(top_srcdir)/.git" git log --stat > $@; \
	fi

dist: ChangeLog

.PHONY: ChangeLog

-include $(top_srcdir)/git.mk
