#!/bin/bash

# fill it
pkgname=mate-indicator-applet
pkgver=2011.12.01
pkgrel=1
pkgdesc="The MATE Indicator Applet"
#depends="gstreamer0.10-base-plugins, mate-panel, mate-character-map, libgtop, libmatenotify, cpufrequtils"
# editar esta funcion!

curdir=`pwd`
srcdir=${curdir}/../..
pkgdir=${curdir}/pkg-content
echo ${srcdir}
cd ${srcdir}

make clean

rm -rf ${pkgdir}/*

./configure --prefix=/usr --sysconfdir=/etc --disable-static \
		--localstatedir=/var --disable-scrollkeeper || return 1

make || return 1

make MATECONF_DISABLE_MAKEFILE_SCHEMA_INSTALL=1 DESTDIR="${pkgdir}" install || return 1

install -m755 -d "${pkgdir}/usr/share/mateconf/schemas"
mateconf-merge-schema "${pkgdir}/usr/share/mateconf/schemas/${pkgname}.schemas" --domain ${pkgname} ${pkgdir}/etc/mateconf/schemas/*.schemas || return 1
rm -f ${pkgdir}/etc/mateconf/schemas/*.schemas

cd ${curdir}
dpkg-buildpackage
