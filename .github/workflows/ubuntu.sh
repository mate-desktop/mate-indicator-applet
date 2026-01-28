#!/usr/bin/bash

# Use grouped output messages
infobegin() {
	echo "::group::${1}"
}
infoend() {
	echo "::endgroup::"
}

# Required packages on Ubuntu
requires=(
	ccache # Use ccache to speed up build
)

# https://git.launchpad.net/ubuntu/+source/mate-indicator-applet/tree/debian/control
requires+=(
	autoconf-archive
	autopoint
	git
	libgtk-3-dev
	libido3-0.1-dev
	libindicator3-dev
	libmate-panel-applet-dev
	libtool
	libx11-dev
	libxml2-dev
	make
	mate-common
)

infobegin "Update system"
apt-get update -y
infoend

infobegin "Install dependency packages"
env DEBIAN_FRONTEND=noninteractive \
	apt-get install --assume-yes \
	${requires[@]}
infoend
