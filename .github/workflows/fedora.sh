#!/usr/bin/bash

set -eo pipefail

# Use grouped output messages
infobegin() {
	echo "::group::${1}"
}
infoend() {
	echo "::endgroup::"
}

# Required packages on Fedora
requires=(
	ccache # Use ccache to speed up build
)

# https://src.fedoraproject.org/cgit/rpms/mate-indicator-applet.git
requires+=(
	autoconf-archive
	gcc
	git
	gtk3-devel
	libindicator-gtk3-devel
	make
	mate-common
	mate-panel-devel
	redhat-rpm-config
)

infobegin "Update system"
dnf update -y
infoend

infobegin "Install dependency packages"
dnf install -y ${requires[@]}
infoend
