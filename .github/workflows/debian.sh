#!/usr/bin/bash

# Use grouped output messages
infobegin() {
	echo "::group::${1}"
}
infoend() {
	echo "::endgroup::"
}

# Required packages on Debian
requires=(
	ccache # Use ccache to speed up build
)

# https://salsa.debian.org/debian-mate-team/mate-indicator-applet
requires+=(
	autoconf-archive
	autopoint
	gcc
	git
	libgtk-3-dev
	libayatana-ido3-dev
	libayatana-indicator3-dev
	libmate-panel-applet-dev
	libtool
	libx11-dev
	libxml2-dev
	make
	mate-common
)

infobegin "Update system"
apt-get update -qq
infoend

infobegin "Install dependency packages"
env DEBIAN_FRONTEND=noninteractive \
	apt-get install --assume-yes \
	${requires[@]}
infoend
