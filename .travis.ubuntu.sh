#! /bin/bash

set -e -x

make -f Makefile.repo

# convert the OBS Debian files to the native Debian files
(cd debian && rename 's/^debian\.//' debian.*)

# build binary packages
dpkg-buildpackage -j`nproc` -rfakeroot -b

# install the built packages
dpkg -i ../*.deb

# smoke test, make sure snapper at least starts
snapper --version

